#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>
#include "secrets.h"
#include "MqttHandler.h"
#include "SensorManager.h"
#include "LCDHandler.h"
#include "AlarmHandler.h"

// LED RGB
#define LED_ONBOARD LED_BUILTIN_AUX
// D0, LED on the development board (between the ESP module and the USB port)
//https://github.com/nodemcu/nodemcu-devkit-v1.0/blob/master/NODEMCU_DEVKIT_V1.0.PDF

//BUTTON
#define RESET_ALARMS D5
#define BUTTON_DEBOUNCE_DELAY 200
unsigned long lastDebounceTime = 0;  // L'ultima volta che il pin è stato campionato
bool lastButtonState = LOW;

#define RSSI_THRESHOLD -80

// WiFi config

char ssid[] = SECRET_SSID;  // your network SSID (name)
char pass[] = SECRET_PASS;  // your network password
#ifdef IP
IPAddress ip(IP);
IPAddress subnet(SUBNET);
IPAddress dns(DNS);
IPAddress gateway(GATEWAY);
#endif
WiFiClient client;

// InfluxDB cfg
InfluxDBClient client_idb(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

// MQTT Broker settings
MqttHandler mqtt(client, "broker.emqx.io", 1883);

//Sensori
SensorManager sensor;

//LCD
LCDHandler lcd;

// Alarm LEDRGB
AlarmHandler alarm;

Ticker tickerBlink;
Ticker writeToInflux;
Ticker writeLCD;
Ticker tickerAlarm;


volatile bool flagWriteInflux = false;

void callback(char* topic, byte* payload, unsigned int length) {
  mqtt.processMessage(topic, payload, length);
}

void setup() {
  Serial.begin(115200);
  long _dc = connectToWiFi();
  pinMode(RESET_ALARMS, INPUT_PULLUP);

  mqtt.begin(callback);
  lcd.begin();
  sensor.begin();
  alarm.begin();

  writeToInflux.attach(20.0, []() {
    flagWriteInflux = true;
  });

  writeLCD.attach(2.0, []() {
    lcd.popAndDisplay();
  });

  tickerAlarm.attach(1.5, []() {
    alarm.nextAlarmColor();
  });
}

void loop() {
  mqtt.handle();
  if (!mqtt.isRunning()) {
    lcd.addMessage("Status", "OFFLINE");
    return;
  } else {
    lcd.addMessage("Status", "ONLINE");
  }

  if (flagWriteInflux) {
    flagWriteInflux = false;
    sendDataToInflux();
  }

  int reading = digitalRead(RESET_ALARMS);
  if (reading != lastButtonState) {
    // Reset del timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE_DELAY) {
    // Se è passato abbastanza tempo, la lettura è stabile
    if (reading == LOW) {
      alarm.flipEnabled();
      const char* aStatus = alarm.getAlarmStatus() ? "ON" : "OFF";
      lcd.addMessage("Alarm Status:", aStatus);
    }
  }

  lastButtonState = reading;
}

// Influx DB operations

bool check_influxdb() {
  // check InfluxDB server connection
  if (client_idb.validateConnection()) {
    Serial.print(F("Connected to InfluxDB: "));
    Serial.println(client_idb.getServerUrl());
    return true;
  } else {
    Serial.print(F("InfluxDB connection failed: "));
    Serial.println(client_idb.getLastErrorMessage());
    return false;
  }
}

void sendDataToInflux() {

  if (!check_influxdb()) {
    Serial.println(F("Not Connected To InfluxDB"));
    return;
  }

  Serial.print(F("Sending to InfluxDB... "));

  Point sensorData("Serra");
  sensorData.addTag("device", "NodeMCU");

  Thresholds currentThr = mqtt.getThresholds();

  // Controlla se il primo carattere NON è lo zero (stringa non vuota)
  if (currentThr.platName[0] != '\0') {
    sensorData.addTag("pianta", currentThr.platName);
  } else {
    Serial.println(F("Plant Name not Found"));
    alarm.addAlarm(AlarmType::SENSOR_ERROR);
    lcd.addMessage("ERROR", "Plant null");
    return;
  }

  PlantData data = sensor.getAllData();
  if (!data.valid) {
    Serial.println(F("Datas are not valid"));
    alarm.addAlarm(AlarmType::SENSOR_ERROR);
    lcd.addMessage("ERROR", "Sensors Error");
    return;
  }


  bool tempInRange = data.temperature >= currentThr.tempMin && data.temperature <= currentThr.tempMax;
  bool humInRange = data.temperature >= currentThr.humMin && data.temperature <= currentThr.humMax;
  bool luxInrange = data.light >= currentThr.luxMin && data.light <= currentThr.luxMax;

  if (tempInRange && humInRange && luxInrange) {
    alarm.addAlarm(AlarmType::ALL_OK);
    // Se tutto è OK, potresti voler rimuovere gli altri errori
    alarm.removeAlarm(AlarmType::SOME_THRESHOLDS_OUT);
    alarm.removeAlarm(AlarmType::ALL_THRESHOLDS_OUT);
    lcd.addMessage("Thresholds", "ALL OK");
  } else if (!tempInRange && !humInRange && !luxInrange) {
    alarm.addAlarm(AlarmType::ALL_THRESHOLDS_OUT);
    alarm.removeAlarm(AlarmType::ALL_OK);
    alarm.removeAlarm(AlarmType::SOME_THRESHOLDS_OUT);
    lcd.addMessage("Thresholds", "ALL O.O.R");
  } else {
    alarm.addAlarm(AlarmType::SOME_THRESHOLDS_OUT);
    alarm.removeAlarm(AlarmType::ALL_OK);
    alarm.removeAlarm(AlarmType::ALL_THRESHOLDS_OUT);
    lcd.addMessage("Thresholds", "SOME O.O.R");
  }

  long rssi = connectToWiFi();
  if (rssi < RSSI_THRESHOLD) {
    Serial.println(F("RSSI too low"));
    Serial.println(rssi);
    return;
  }

  sensorData.addField("temp", data.temperature);
  sensorData.addField("hum", data.humidity);
  sensorData.addField("lux", data.light);
  sensorData.addField("rssi", rssi);

  if (client_idb.writePoint(sensorData)) {
    Serial.print(F("Sent to data to influxDB"));
    lcd.addMessagePlantData(data.temperature, data.humidity, data.light);
  } else {
    Serial.println(client_idb.getLastErrorMessage());
  }
}

// WiFi connection
long connectToWiFi() {
  long rssi_strength;
  // connect to WiFi (if not already connected)
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("Connecting to SSID: "));
    Serial.println(ssid);

#ifdef IP
    WiFi.config(ip, dns, gateway, subnet);  // by default network is configured using DHCP
#endif

    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(F("."));
      delay(250);
    }
    Serial.println(F("\nConnected!"));
    rssi_strength = WiFi.RSSI();  // get wifi signal strength
  } else {
    rssi_strength = WiFi.RSSI();  // get wifi signal strength
  }

  return rssi_strength;
}
