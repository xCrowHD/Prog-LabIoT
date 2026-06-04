#ifdef ARDUINO
  #include <Ticker.h>
  #include <ESP8266WiFi.h>
  #define RESET_ALARMS D5
  #define LED_RED D0
  #define LED_GREEN D4
  #define LED_BLUE D3
#else
  #include "MockLibraries/Ticker.h"
  #include "MockLibraries/ESP8266WiFi.h"
  #define RESET_ALARMS 5
  #define LED_RED 0
  #define LED_GREEN 4
  #define LED_BLUE 3
#endif

#include "InfluxHandler.h"
#include "secrets.h"
#include "MqttHandler.h"
#include "SensorManager.h"
#include "LCDHandler.h"
#include "AlarmHandler.h"
#include "WiFiHandler.h"
#include "HandleExceptions.h"


// D0, LED on the development board (between the ESP module and the USB port)
//https://github.com/nodemcu/nodemcu-devkit-v1.0/blob/master/NODEMCU_DEVKIT_V1.0.PDF

//BUTTON
#define RSSI_THRESHOLD -80
#define BUTTON_DEBOUNCE_DELAY 20
unsigned long lastDebounceTime = 0;  // L'ultima volta che il pin è stato campionato
bool lastButtonState = HIGH;
float sampleAfterAlarmDisabling = 25.0f; // Intervallo di tempo dopo il quale Alarm torna sensibile alla raccolta degi errori

// WiFi config
WiFiClient client;

// InfluxDB cfg
InfluxHandler client_idb(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

// MQTT Broker settings
MqttHandler mqtt;

//Sensori
SensorManager sensor;

//LCD
LCDHandler lcd;

// LED
LED led = {
  .r = LED_RED, 
  .g = LED_GREEN, 
  .b = LED_BLUE};

// Alarm LEDRGB
AlarmHandler alarm(lcd, led);
Ticker writeToInflux;
Ticker tickerBlink;
Ticker writeLCD;
Ticker tickerAlarm;
Ticker idTick;
Ticker checkAlarmStatus;

HandleExceptions checkStatus(alarm, client_idb);

float lastTimerValue = 20.0;
char id[13];

volatile bool flagCheckSensor;


void setup() {
  Serial.begin(115200);
  WiFiHandler::begin();
  WiFiHandler::getMacAddress(id);
  pinMode(RESET_ALARMS, INPUT_PULLUP);

  mqtt.begin(client, "broker.emqx.io", 1883);
  lcd.begin();
  sensor.begin();
  alarm.begin();
  client_idb.begin(lastTimerValue);

  writeLCD.attach(2.0, []() {
    lcd.popAndDisplay();
  });

  tickerAlarm.attach(1.5, []() {
    alarm.nextAlarm();
  });

  idTick.attach(5.0, []{
    lcd.addMessage("ID:", id, MessageType::INFO);
  });

  checkAlarmStatus.attach(5.0, [](){
    flagCheckSensor = true;
  });

}

void loop() {
  mqtt.handle();
  if (mqtt.isStandBy()){
    lcd.addMessage("Status", "StandByMode", MessageType::INFO);
    return;
  }

  if (!mqtt.isSet()) {
    lcd.addMessage("Status", "Need Settings", MessageType::INFO);
    return;
  }


  if (!mqtt.isRunning()) {
    lcd.addMessage("Status", "OFFLINE", MessageType::INFO);
    return;
  } else {
    lcd.addMessage("Status", "ONLINE", MessageType::INFO);
  }

  float mqttTimer = mqtt.getSettings().timer;
  if (mqttTimer != lastTimerValue) {
    client_idb.updateInterval(mqttTimer);
    lastTimerValue = mqttTimer;
  }
    if (flagCheckSensor) {
        flagCheckSensor = false; 

        long rssi = WiFiHandler::getRSSI();
        PlantData data = sensor.getAllData();
        Thresholds currentThr = mqtt.getThresholds();

        // I singoli metodi qui sotto aggiungono o rimuovono gli allarmi in autonomia
        bool connStatus = checkStatus.handleConnectionException(rssi, RSSI_THRESHOLD);
        bool dataStatus = checkStatus.handleDataException(data);
        bool mqttStatus = checkStatus.handleMqttExceptions(currentThr);
        bool thrStatus  = checkStatus.handleThresholds(data, currentThr);
        
        if (dataStatus){
          lcd.addMessagePlantData(data.temperature, data.humidity, data.light);
        }
        // Gestione InfluxDB
        InfluxStatus status = InfluxStatus::SUCCESS;

        if (client_idb.isReadyToWrite() && connStatus && dataStatus) {
            status = client_idb.sendDataToInflux(data, rssi, "Serra", "NodeMCU", currentThr);
        }
        
        bool influxStatus = checkStatus.handleInfluxException(status);


        // Valutazione dello stato globale per aggiornare scritte LCD di successo
        if (connStatus && dataStatus && mqttStatus && thrStatus && influxStatus) {
            checkStatus.handleSuccess(); 
        }
        else{
          alarm.nextAlarm();
        }
      }
  
     
  int reading = digitalRead(RESET_ALARMS);
  if (reading != lastButtonState) {
    // Reset del timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE_DELAY) {
    static bool wasAlreadyPressed = false;
    // Se è passato abbastanza tempo, la lettura è stabile
    if (reading == LOW && !wasAlreadyPressed) {   
      alarm.setAllAlarmAcked();
      wasAlreadyPressed = true;   
    }
    if (reading == HIGH) {
      // Quando rilasci il bottone, resettiamo la guardia per la prossima volta
      wasAlreadyPressed = false;
    }
  }

  lastButtonState = reading;
}
