#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>
#include "secrets.h"
#include "MqttHandler.h"
#include "SensorManager.h"
#include "LCDHandler.h"
#include "AlarmHandler.h"

// LED RGB
#define LED_RED D8  // In conflitto con DHT, usare switch o cambiare pin
#define LED_GREEN D6
#define LED_BLUE D3
#define LED_ONBOARD LED_BUILTIN_AUX  // D0, LED on the development board (between the ESP module and the USB port)  https://github.com/nodemcu/nodemcu-devkit-v1.0/blob/master/NODEMCU_DEVKIT_V1.0.PDF
#define BUTTON D4 // numero a caso per farlo girare

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

//Ticker dell'allarme gestito internamente in Alarmhandler.cpp
Ticker writeToInflux;

AlarmHandler alarm(LED_RED, LED_GREEN, LED_BLUE);

volatile bool flagWriteInflux = false;

void callback(char* topic, byte* payload, unsigned int length) {
  mqtt.processMessage(topic, payload, length);
}

void setup() {
  long _dc = connectToWiFi();
  Serial.begin(115200);
  mqtt.begin(callback);
  lcd.begin();
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  
  sensor.begin();

  alarm.setAlarmType(NONE);
  alarm.manageLEDerrors(); //questa configurazione spegne tutti i led

  writeToInflux.attach(20.0, []() {
    flagWriteInflux = true;
  });
}

void loop() {
  mqtt.handle();
  if (!mqtt.isRunning()) {
    Serial.println(F("ESP8266 OFFMODE"));
    lcd.displayMessage("OFFMODE");
    delay(1000);
    return;
  }

  if (flagWriteInflux) {
    flagWriteInflux = false;
    sendDataToInflux();
  }
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
    Serial.println(F("Not connected To InfluxDB"));
    return;
  }

  Serial.print(F("Sending to InfluxDB... "));

  Point sensorData("Greenhouse");
  sensorData.addTag("device", "NodeMCU");

  Thresholds currentThr = mqtt.getThresholds();


  if (currentThr.platName[0] != '\0') { //da spostare lato MQTT
    sensorData.addTag("Plant", currentThr.platName);
  } else {
    Serial.println(F("Plant Name not Found"));
    return;
  }

  PlantData data = sensor.getAllData();


  if (!data.valid) {
    alarm.setAlarmType(SENSOR_ERROR);
    alarm.manageLEDerrors();
    Serial.println(F("Datas are not valid"));
    return;
  }

  bool tempInRange = data.temperature >= currentThr.tempMin && data.temperature <= currentThr.tempMax;
  bool humInRange = data.temperature >= currentThr.humMin && data.temperature <= currentThr.humMax;
  bool luxInrange = data.light >= currentThr.luxMin && data.light <= currentThr.luxMax; 

  if (tempInRange && humInRange && luxInrange)
  {
    alarm.setAlarmType(ALL_OK);
  }
  else if (!tempInRange && !humInRange && !luxInrange)
  {
    alarm.setAlarmType(ALL_THRESHOLDS_OUT);
  }
  else 
  {
    alarm.setAlarmType(SOME_THRESHOLDS_OUT);
  }
  
  alarm.manageLEDerrors();
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
    lcd.displayData(data.temperature, data.humidity, data.light);
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
