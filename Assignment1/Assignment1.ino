#include <DHT.h>
#include <LiquidCrystal_I2C.h>   // display library
#include <Wire.h>                // I2C library

#include <Ticker.h>
#include <ESP8266WiFi.h>

#include <InfluxDbClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "secrets.h"

// Struct 
struct WeatherData{
  float temperature;
  float humidity;
  float hic;
  float light;
  bool valid;
};

struct Thresholds{
  float tMin;
  float tMax;
  float hMin;
  float hMax;
  float lMin;
  float lMax;
};


struct AlarmStatus{
  bool dhtError = false;
  bool lightError = false;
  bool silenced = false;
};

enum AlarmType {NONE, ALL_OK, THRESHOLDS_OUT, SENSOR_ERROR};


// Button
AlarmStatus currentAlarm;
#define BUTTON D4 //Messo un numero a caso per farlo compilare

// Display 
#define DISPLAY_CHARS 16    // number of characters on a line
#define DISPLAY_LINES 2     // number of display lines
#define DISPLAY_ADDR 0x27   // display address on I2C bus

// DHT sensor
#define DHTPIN D7     // sensor I/O pin, eg. D1 (DO NOT USE D0 or D4! see above notes)
#define DHTTYPE DHT11  // sensor type DHT 11

// Photoresistor
#define PHOTORESISTOR A0 

// LED RGB
#define LED_RED D8   // D8 attaccato a ground! potrebbe causare errori
#define LED_GREEN D6
#define LED_BLUE D3

// Influx 
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

// Display cfg
LiquidCrystal_I2C lcd(DISPLAY_ADDR, DISPLAY_CHARS, DISPLAY_LINES);

// DHT config
DHT dht = DHT(DHTPIN, DHTTYPE);

// MQTT Broker config
const char *mqtt_broker = "broker.emqx.io";  // EMQX broker endpoint
const char *mqtt_topic_threshold = "lab_iot/mafogani/threshold";  // MQTT topic
const int mqtt_port = 1883;  // MQTT port (TCP)
PubSubClient mqtt_client(client);

char plantName[30] = "";

Ticker tickerBlink;   
Ticker writeToInflux;

volatile bool flagWriteInflux = false;
int blinkRemaining = 0;
int currentBlinkPin = -1;

AlarmType lastAlarmType = NONE;
bool alarmSilenced = false;
Thresholds ts; //Non mi piace threshold globale, ma per ora teniamo così e vediamo se funziona

void displayLCDconfig();

void setup() {

  Serial.begin(115200);
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  ledOff();

  Wire.begin();
  bool lcdFound = false;
  int trials = 0;
  
  while (!lcdFound) {
    Wire.beginTransmission(DISPLAY_ADDR);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.println(F("LCD found!"));
      lcd.begin(DISPLAY_CHARS, 2);
      displayLCDconfig();
      lcdFound = true;
    } else {
      trials++;
      Serial.print(F("LCD not found (error "));
      Serial.print(error);
      Serial.print(F("). Trial n. "));
      Serial.println(trials);
      
      for(int i = 0; i < 10; i++) {
        delay(100); 
        yield(); 
      }
    }

    if (trials % 10 == 0 && !lcdFound) {
       startBlink(LED_RED, 5);
       lcdFound = true; 
       Serial.println(F("LCD not found. Continuing without it!")); 
    }
  }

  dht.begin();

  writeToInflux.attach(5.0, []() { flagWriteInflux = true; });
  Serial.println("\nSetup completed!\n");
}

void loop() {
  static WeatherData dataToSend;
  connectToWiFi();
  
  if (!mqtt_client.connected()) {
    reconnectMQTT();
  }

  mqtt_client.loop();

  if (flagWriteInflux)
  {
    flagWriteInflux = false;
    dataToSend.valid = true;
    readDHT(dataToSend);
    readPR(dataToSend);
    sendDataToInflux(dataToSend);
    showDataOnDisplay(dataToSend);
    updateLEDStatus(dataToSend, ts); 
  }
}

// Display
void displayLCDconfig() {
  lcd.setBacklight(255);
  lcd.home();
  lcd.clear();
  lcd.print("Hello LCD!");
  delay(1000); // Nel setup il delay è accettabile e più sicuro
  lcd.clear();
}

void showDataOnDisplay(WeatherData &data)
{
  float t = data.temperature;
  float h = data.humidity;
  float hic = data.hic;
  float l = data.light;

  lcd.setCursor(0, 0);
  lcd.print("H: ");
  lcd.print(int(h));
  lcd.print("% T:");
  lcd.print(int(t));
  lcd.print(" C");
  lcd.setCursor(0, 1);
  lcd.print("A.temp: ");   // the temperature perceived by humans (takes into account humidity)
  lcd.print(hic);
  lcd.print(" C");
  lcd.print("L:");
  lcd.print(int(l));
  lcd.print("/1023"); 
}

// DHT functions
void readDHT(WeatherData &data)
{
  float h = dht.readHumidity();      // humidity percentage, range 20-80% (±5% accuracy)
  float t = dht.readTemperature();   // temperature Celsius, range 0-50°C (±2°C accuracy)
  if (isnan(h) || isnan(t)) 
  {   
    Serial.println(F("Failed to read from DHT sensor!"));
    startBlink(LED_RED  , 3);
    data.valid = false;
    return;
  }

  data.temperature = t;
  data.humidity= h;
  data.hic = dht.computeHeatIndex(t, h, false);

  return;
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

void sendDataToInflux(WeatherData &data) {
  // Invece di validare la connessione ogni volta, 
  // controlliamo solo se il Wi-Fi è attivo
  if (WiFi.status() != WL_CONNECTED || !data.valid) return;

  Point sensorData("Greenhouse");
  sensorData.addTag("device", "NodeMCU");
  sensorData.addTag("plant", (plantName[0] == '\0') ? "Unknown" : plantName);
  
  sensorData.addField("temp", data.temperature);
  sensorData.addField("hum", data.humidity);
  sensorData.addField("lux", data.light);
  sensorData.addField("rssi", WiFi.RSSI());

  if (client_idb.writePoint(sensorData)) {
    startBlink(LED_BLUE, 1);
  } else {
    Serial.print(F("InfluxDB Error: "));
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

// Phororesistor
void readPR(WeatherData &data)
{
  data.light = analogRead(PHOTORESISTOR);   // read analog value (range 0-1023)
  Serial.print(F("Light sensor value: "));
  Serial.println(data.light);
}


// MQTT
void reconnectMQTT() {
  static unsigned long lastRetry = 0;
  if (!mqtt_client.connected() && (millis() - lastRetry > 5000)) {
    lastRetry = millis();
    Serial.print("Attempting MQTT connection...");
    if (mqtt_client.connect("ESP8266_Serra_Client")) {
      Serial.println("Connected!");
      mqtt_client.subscribe(mqtt_topic_threshold);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
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

void setThresholds(byte *payload, unsigned int length) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print(F("JSON errore parsing "));
    Serial.println(error.f_str());
    return;
  }

  ts.tMin = doc["temp_min"] | ts.tMin;
  ts.tMax = doc["temp_max"] | ts.tMax;
  ts.hMin = doc["hum_min"] | ts.hMin;
  ts.hMax = doc["hum_max"] | ts.hMax;
  ts.lMin = doc["lux_min"] | ts.lMin;
  ts.lMax = doc["lux_max"] | ts.lMax;

  
  if (doc.containsKey("plant")) {
    strlcpy(plantName, doc["plant"], sizeof(plantName));
    Serial.print(F("Set new plant: "));
    Serial.println(plantName);
  }

  // IMPORTANTE: Reset dello stato allarme
  // Ricevere nuove soglie equivale a un "cambio di scenario", 
  // quindi forziamo il sistema a ricalcolare i LED al prossimo loop.
  lastAlarmType = NONE; 
  alarmSilenced = false;
  
  Serial.println(F("Thresholds updated via MQTT."));
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

void updateLEDStatus(WeatherData &data, Thresholds &ts) {
  AlarmType currentType;

  // 1. Determina il tipo di allarme attuale
  if (!data.valid) {
    currentType = SENSOR_ERROR;
  } else {
    bool tIn = (data.temperature >= ts.tMin && data.temperature <= ts.tMax);
    bool hIn = (data.humidity >= ts.hMin && data.humidity <= ts.hMax);
    bool lIn = (data.light >= ts.lMin && data.light <= ts.lMax);

    if (tIn && hIn && lIn) {
      currentType = ALL_OK;
    } else {
      currentType = THRESHOLDS_OUT;
    }
  }

  // 2. Se il tipo di allarme è cambiato rispetto all'ultima volta, riattiva i LED
  if (currentType != lastAlarmType) {
    alarmSilenced = false;
    lastAlarmType = currentType;
    tickerBlink.detach(); // Ferma eventuali lampeggi precedenti
  }

  // 3. Esecuzione visiva (solo se non silenziato)
  if (alarmSilenced) {
    ledOff();
    return;
  }

  switch (currentType) {
    case SENSOR_ERROR:
      startBlink(LED_RED, 999); // Lampeggio continuo (count alto)
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE, LOW);
      break;

    case THRESHOLDS_OUT:
      tickerBlink.detach();
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE, LOW);
      break;

    case ALL_OK:
      tickerBlink.detach();
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_BLUE, LOW);
      break;
      
    case NONE:
      ledOff();
      break;
  }
}