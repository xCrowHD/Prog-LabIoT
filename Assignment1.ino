#include <DHT.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

struct TempHum {
  float temperatura;
  float umidita;
  bool valid;
};

// DHT sensor
#define DHTPIN D7     // sensor I/O pin, eg. D1 (DO NOT USE D0 or D4! see above notes)
#define DHTTYPE DHT11  // sensor type DHT 11

// Photoresistor
#define PHOTORESISTOR A0 

// LED RGB
#define LED_RED D8   // In conflitto con DHT, usare switch o cambiare pin
#define LED_GREEN D6
#define LED_BLUE D3

// Influx settings
#define LED_ONBOARD LED_BUILTIN_AUX   // D0, LED on the development board (between the ESP module and the USB port)  https://github.com/nodemcu/nodemcu-devkit-v1.0/blob/master/NODEMCU_DEVKIT_V1.0.PDF
#define RSSI_THRESHOLD -60 

// WiFi config

char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password
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
const char *mqtt_broker = "broker.emqx.io";  // EMQX broker endpoint
const char *mqtt_topic_threshold = "lab_iot/mafogani/threshold";  // MQTT topic
const int mqtt_port = 1883;  // MQTT port (TCP)
PubSubClient mqtt_client(client);

char plantName[30] = "";

void toggleLedRed(int times);

Ticker tickerBlink;   
Ticker writeToInflux;

volatile bool flagWriteInflux = false;
int blinkRemaining = 0;
int currentBlinkPin = -1;

DHT dht = DHT(DHTPIN, DHTTYPE);

void setup() {

  Serial.begin(115200);
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  ledOff();

  dht.begin();

  writeToInflux.attach(5.0, []() { flagWriteInflux = true; });

}

void loop() {

  long rssi_strength = connectToWiFi();
  if (!mqtt_client.connected()) {
    reconnectMQTT();
  }
  mqtt_client.loop();

  if (flagWriteInflux)
  {
    flagWriteInflux = false;
    sendDataToInflux();
  }

}

// DHT functions
TempHum readDHT()
{
  float h = dht.readHumidity();      // humidity percentage, range 20-80% (±5% accuracy)
  float t = dht.readTemperature();   // temperature Celsius, range 0-50°C (±2°C accuracy)
  TempHum data;
  if (isnan(h) || isnan(t)) 
  {   // readings failed, skip
    Serial.println(F("Failed to read from DHT sensor!"));
    startBlink(LED_RED, 2);
    data.valid = false;
    return data;
  }
  
  Serial.print(F("\nHumidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);

  data.temperatura = t;
  data.umidita = h;
  data.valid = true;
  return data;
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

  if (!check_influxdb()){
    Serial.println(F("Not Connected To InfluxDB"));
    return;
  }

  Serial.print(F("Sending to InfluxDB... "));

  Point sensorData("Serra");
  sensorData.addTag("device", "NodeMCU");
  sensorData.addTag("pianta", plantName);

  TempHum dht = readDHT();
  if(!dht.valid){
    return;
  }

  int pr = readPR();
  if ( pr == -1){
    return;
  }

  long rssi = connectToWiFi();
  if(rssi < RSSI_THRESHOLD){
    return;
  }

  sensorData.addField("temp", dht.temperatura);
  sensorData.addField("hum", dht.umidita);
  sensorData.addField("lux", pr);
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
    WiFi.config(ip, dns, gateway, subnet);   // by default network is configured using DHCP
#endif

    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(F("."));
      delay(250);
    }
    Serial.println(F("\nConnected!"));
    rssi_strength = WiFi.RSSI();   // get wifi signal strength
  } else {
    rssi_strength = WiFi.RSSI();   // get wifi signal strength
  }

  return rssi_strength;
}

int readPR(){
  
  static unsigned int lightSensorValue;

  lightSensorValue = analogRead(PHOTORESISTOR);   // read analog value (range 0-1023)
  Serial.print(F("Light sensor value: "));
  Serial.println(lightSensorValue);
  if ( lightSensorValue == 0){
    return -1;
  }
  return lightSensorValue;
}

// MQTT
void reconnectMQTT() {
  while (!mqtt_client.connected()) {
    Serial.print("Tentativo connessione MQTT...");
    // Tenta di connettersi con un ID univoco
    if (mqtt_client.connect("ESP8266_Serra_Client")) {
      Serial.println("Connesso!");
      mqtt_client.subscribe(mqtt_topic_threshold);
    } else {
      delay(250);
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.println(topic);

  if (strcmp(topic, mqtt_topic_threshold) == 0){
    setThresholds(payload, length);
  }
  else{
    Serial.print("Message:");

    for (unsigned int i = 0; i < length; i++) {
      Serial.print((char) payload[i]);
    }

    Serial.println();
    Serial.println("-----------------------");
  }
}

void setThresholds(byte *payload, unsigned int length){
  StaticJsonDocument<256> doc;

  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print(F("Errore parsing JSON: "));
    Serial.println(error.f_str());
    return;
  }

  float t_max = doc["temp_max"];
  int h_min   = doc["hum_min"];
  const char* nome = doc["pianta"];
}
