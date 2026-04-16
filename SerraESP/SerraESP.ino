#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "MqttHandler.h"
#include "SensorManager.h"

// LED RGB
#define LED_RED D8  // In conflitto con DHT, usare switch o cambiare pin
#define LED_GREEN D6
#define LED_BLUE D3
#define LED_ONBOARD LED_BUILTIN_AUX  // D0, LED on the development board (between the ESP module and the USB port)  https://github.com/nodemcu/nodemcu-devkit-v1.0/blob/master/NODEMCU_DEVKIT_V1.0.PDF

#define RSSI_THRESHOLD -60

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

char plantName[30] = "";

void toggleLedRed(int times);

Ticker tickerBlink;
Ticker writeToInflux;

volatile bool flagWriteInflux = false;
int blinkRemaining = 0;
int currentBlinkPin = -1;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Messaggio ricevuto su [%s]\n", topic);

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (!error) {
    Serial.print("\nAbbiamo un Messaggio");
  }
}

void setup() {

  Serial.begin(115200);
  mqtt.begin(callback);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  sensor.begin();
  ledOff();

  writeToInflux.attach(5.0, []() {
    flagWriteInflux = true;
  });
}

void loop() {

  mqtt.handle();

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
  sensorData.addTag("pianta", plantName);

  PlantData data = sensor.getAllData();
  if (!data.valid) {
    return;
  }

  long rssi = connectToWiFi();
  if (rssi < RSSI_THRESHOLD) {
    return;
  }

  sensorData.addField("temp", data.temperatura);
  sensorData.addField("hum", data.umidita);
  sensorData.addField("lux", data.luce);
  sensorData.addField("rssi", rssi);

  if (client_idb.writePoint(sensorData)) {
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
