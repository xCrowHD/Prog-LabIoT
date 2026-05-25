#include <Ticker.h>
#include <ESP8266WiFi.h>
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
#define RESET_ALARMS D5
#define BUTTON_DEBOUNCE_DELAY 20
unsigned long lastDebounceTime = 0;  // L'ultima volta che il pin è stato campionato
bool lastButtonState = HIGH;

#define RSSI_THRESHOLD -80

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

// Alarm LEDRGB
AlarmHandler alarm;
Ticker writeToInflux;
Ticker tickerBlink;
Ticker writeLCD;
Ticker tickerAlarm;
Ticker idTick;
Ticker checkAlarmStatus;

HandleExceptions checkStatus(alarm, lcd, client_idb);

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
    alarm.nextAlarmColor();
  });

  idTick.attach(5.0, []{
    lcd.addMessage("ID:", id);
  });

  checkAlarmStatus.attach(5.0, [](){
    flagCheckSensor = true;
  });

}

void loop() {
  mqtt.handle();
  if (mqtt.isStandBy()){
    lcd.addMessage("Status", "StandByMode");
    return;
  }

  if (!mqtt.isSet()) {
    lcd.addMessage("Status", "Need Settings");
    return;
  }
  if (!mqtt.isRunning()) {
    lcd.addMessage("Status", "OFFLINE");
    return;
  } else {
    lcd.addMessage("Status", "ONLINE");
  }

  float mqttTimer = mqtt.getSettings().timer;
  if (mqttTimer != lastTimerValue) {
    client_idb.updateInterval(mqttTimer);
    lastTimerValue = mqttTimer;
  }
  
  long rssi = WiFiHandler::getRSSI();
  PlantData data = sensor.getAllData();
  Thresholds currentThr = mqtt.getThresholds();

  bool connectionOK = (rssi >= RSSI_THRESHOLD); 
  bool dataOK = data.valid;

  if (client_idb.isReadyToWrite() && connectionOK && dataOK){
      InfluxStatus status = client_idb.sendDataToInflux(data, rssi, "Serra", "NodeMCU", currentThr);
      checkStatus.handleInfluxException(status);
  }  

  if (flagCheckSensor) {
    flagCheckSensor = false; 

    // Eseguiamo TUTTI i controlli in modo indipendente.
    // Ognuno di loro aggiungerà o rimuoverà il proprio allarme specifico.
    long rssi = WiFiHandler::getRSSI();
    bool connStatus   = checkStatus.handleConnectionException(rssi, RSSI_THRESHOLD);
    bool dataStatus   = checkStatus.handleDataException(data);
    bool mqttStatus   = checkStatus.handleMqttExceptions(currentThr);
    bool thrStatus    = true; // Di base assumiamo siano OK, cambieranno solo se controllati
    bool influxStatus = true; 

    // Controlliamo le soglie solo se i dati del sensore sono effettivamente validi
    if (dataStatus) {
      thrStatus = checkStatus.handleThresholds(data, currentThr);
      
      // Controlliamo Influx solo se siamo online
      if (connStatus) {
        InfluxStatus status = client_idb.influxStatus(data, currentThr);
        influxStatus = checkStatus.handleInfluxException(status);
      } else {
        // Se non c'è connessione, non possiamo testare Influx adesso. 
        // Rimuoviamo il vecchio errore Influx per non bloccare la logica futura.
        alarm.removeAlarm(AlarmType::INFLUX_ERROR);
      }
    } else {
      // Se i dati del sensore non sono validi, non possiamo testare le soglie.
      alarm.removeAlarm(AlarmType::SOME_THRESHOLDS_OUT);
      alarm.removeAlarm(AlarmType::ALL_THRESHOLDS_OUT);
    }

    // Rimuoviamo l'ALL_OK preventivamente per aggiornarlo alla fine
    alarm.removeAlarm(AlarmType::ALL_OK);

    // SE tutti i report dei singoli handler dicono che è tutto a posto
    if (connStatus && dataStatus && mqttStatus && thrStatus && influxStatus) {
        checkStatus.handleSuccess();
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
      alarm.flipEnabled();
      const char* aStatus = alarm.getAlarmStatus() ? "ON" : "OFF";
      lcd.addMessage("Alarm Status:", aStatus);
      wasAlreadyPressed = true;
    }
    if (reading == HIGH) {
      // Quando rilasci il bottone, resettiamo la guardia per la prossima volta
      wasAlreadyPressed = false;
    }
  }

  lastButtonState = reading;
}
