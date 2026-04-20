#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>
#include "secrets.h"
#include "MqttHandler.h"
#include "SensorManager.h"
#include "LCDHandler.h"

// LED RGB
#define LED_RED D8  // In conflitto con DHT, usare switch o cambiare pin
#define LED_GREEN D6
#define LED_BLUE D3
#define LED_ONBOARD LED_BUILTIN_AUX  // D0, LED on the development board (between the ESP module and the USB port)  https://github.com/nodemcu/nodemcu-devkit-v1.0/blob/master/NODEMCU_DEVKIT_V1.0.PDF

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


void toggleLedRed(int times);

Ticker tickerBlink;
Ticker writeToInflux;
Ticker writeLCD;

volatile bool flagWriteInflux = false;
int blinkRemaining = 0;
int currentBlinkPin = -1;

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
  ledOff();

  writeToInflux.attach(20.0, []() {
    flagWriteInflux = true;
  });
  
  writeLCD.attach(2.0, []() {
    lcd.popAndDisplay();
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
}


//LED functions
void ledOff() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);
}

void startBlink(int pin, int count) {
  currentBlinkPin = pin;
  blinkRemaining = count * 2;
  tickerBlink.attach(0.2, handleBlink);
}

void handleBlink() {
  if (blinkRemaining > 0) {
    digitalWrite(currentBlinkPin, !digitalRead(currentBlinkPin));
    blinkRemaining--;
  } else {
    tickerBlink.detach();
    digitalWrite(currentBlinkPin, LOW);
  }
}

void toggleLedBlue() {
  digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));
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
    return;
  }

  PlantData data = sensor.getAllData();
  if (!data.valid) {
    Serial.println(F("Datas are not valid"));
    return;
  }

  long rssi = connectToWiFi();
  if (rssi < RSSI_THRESHOLD) {
    Serial.println(F("RSSI too low"));
    Serial.println(rssi);
    return;
  }

  sensorData.addField("temp", data.temperatura);
  sensorData.addField("hum", data.umidita);
  sensorData.addField("lux", data.luce);
  sensorData.addField("rssi", rssi);

  if (client_idb.writePoint(sensorData)) {
    Serial.print(F("Sent to data to influxDB"));
    lcd.addMessagePlantData(data.temperatura, data.umidita, data.luce);
    startBlink(LED_BLUE, 1);
  } else {
    Serial.println(client_idb.getLastErrorMessage());
    startBlink(LED_RED, 1);
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
