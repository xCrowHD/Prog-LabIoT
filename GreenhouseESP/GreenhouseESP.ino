#include <Ticker.h>
#include <ESP8266WiFi.h>
#define RESET_ALARMS D5
#define LED_RED D0
#define LED_GREEN D4
#define LED_BLUE D3

#include "InfluxHandler.h"
#include "secrets.h"
#include "MqttHandler.h"
#include "SensorManager.h"
#include "LCDHandler.h"
#include "AlarmHandler.h"
#include "WiFiHandler.h"
#include "HandleExceptions.h"

extern "C"
{
#include "gpio.h"
}
// Required for LIGHT_SLEEP_T delay mode
extern "C"
{
#include "user_interface.h"
}

// D0, LED on the development board (between the ESP module and the USB port)
//https://github.com/nodemcu/nodemcu-devkit-v1.0/blob/master/NODEMCU_DEVKIT_V1.0.PDF

//BUTTON
#define RSSI_THRESHOLD -80
#define BUTTON_DEBOUNCE_DELAY 20
unsigned long lastDebounceTime = 0;  // L'ultima volta che il pin è stato campionato
bool lastButtonState = HIGH;

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


Ticker tickerAlarm;
Ticker checkAlarmStatus;

HandleExceptions checkStatus(alarm, client_idb);

// NOTA: Se MQTT invia il tempo in secondi, inizializziamo coerentemente (es: 20 secondi)
uint32_t lastTimerValue = 20*1e3; //20 secondi
uint32_t maxTimerValue = 268434; // 1 ms sotto il limite hardware

char id[13];

volatile bool flagCheckSensor;
volatile bool flagWrite;

unsigned long lastLcdUpdate = 0; //ms
const unsigned long lcdInterval = 2000; //ms

void manageSleepTime(uint32_t sleepTimeMs);
bool updateInfluxInterval(uint32_t newIntervalSeconds);
void keepButtonAlive();

void setup()
{
  Serial.begin(115200);
  WiFiHandler::begin();
  WiFiHandler::getMacAddress(id);
  pinMode(RESET_ALARMS, INPUT_PULLUP);

  mqtt.begin(client, "broker.emqx.io", 1883);
  lcd.begin();
  sensor.begin();
  alarm.begin();

  tickerAlarm.attach(1.5, []() {
    alarm.nextAlarm();
  });

  checkAlarmStatus.attach(5.0, [](){
    flagCheckSensor = true;
  });
}

void loop()
{

  mqtt.handle();
  lcd.addMessage("ID", id, MessageType::INFO); 

  if (millis() - lastLcdUpdate >= lcdInterval){
    lastLcdUpdate = millis();
    lcd.popAndDisplay();
  }

  if (mqtt.isStandBy()){
    lcd.addMessage("Status", "StandByMode", MessageType::INFO);
    return;
  }

  if (!mqtt.isSet()){
    alarm.addAlarm(AlarmType::NEED_SETTINGS);
    alarm.nextAlarm();
    return;
  }
  alarm.removeAlarm(AlarmType::NEED_SETTINGS);

  if(!mqtt.isRunning()){
   lcd.addMessage("Status", "OFFLINE", MessageType::INFO);
   return;
  } else{
    lcd.addMessage("Status", "Online", MessageType::INFO);
  }

  if (flagCheckSensor){
    flagCheckSensor = false;

    long rssi = WiFiHandler::getRSSI();
    PlantData data = sensor.getAllData();
    Thresholds currentThr = mqtt.getThresholds();

    // I singoli metodi aggiungono o rimuovono gli allarmi in autonomia
    bool connStatus = checkStatus.handleConnectionException(rssi, RSSI_THRESHOLD);
    bool dataStatus = checkStatus.handleDataException(data);  
    bool mqttStatus = checkStatus.handleMqttExceptions(currentThr);
    bool thrStatus = checkStatus.handleThresholds(data, currentThr);

    if (dataStatus){
      lcd.addMessagePlantData(data.temperature, data.humidity, data.light);
    }

    // Logica risveglio e invio dati a Influx ESATTAMENTE 1 volta mentre è sveglio
    bool influxStatus = true;

    if (flagWrite){
      InfluxStatus status = InfluxStatus::SUCCESS;

      if (connStatus && dataStatus){
        status = client_idb.sendDataToInflux(data, rssi, "Serra", "NodeMCU", currentThr);
      }
      else{
        status = InfluxStatus::ERR_INFLUX_CONNECTION;
      }

      influxStatus = checkStatus.handleInfluxException(status);

      if (influxStatus){
        // SUCCESSO: Abbassiamo il flag così NON ci riproverà più fino al prossimo risveglio
        flagWrite = false;
      }
    }
    // ---------------------------------

    // Valutazione dello stato globale per l'ingresso in Light Sleep
    if (connStatus && dataStatus && mqttStatus && thrStatus && influxStatus)
    {
      checkStatus.handleSuccess();

      lcd.addMessage("System", "Going to sleep", MessageType::INFO);
      lcd.popAndDisplay();

      mqtt.sendSleepingStatus();

      // -- CICLO DI SVUOTAMENTO PRIMA DELLO SLEEP ---
      // Manteniamo LCD attivo fintanto che non ha mostrato tutti i messaggi una volta
      // anche nel caso di successo immediato dei booleani nel loop

      unsigned long int lastMessageTime = millis();

      while (millis() - lastMessageTime < 3*lcdInterval){ //mostra gli ultimi 3 messaggi
        if (millis() - lastLcdUpdate >= lcdInterval){
          lastLcdUpdate = millis();
          lcd.popAndDisplay();
        }

        keepButtonAlive();
        yield();
      }

      // --- CONFIGURAZIONE DISPLAY PER IL PERIODO DI SLEEP ---
      // La coda è stata mostrata. Adesso stampiamo sul display SOLO l'ID dell'ESP.
      // Questo messaggio rimarrà impresso staticamente sul display per tutta la durata del sonno.
      lcd.clearAll();                               // Se il tuo handler ha un clear, pulisce lo schermo
      lcd.addMessage("ID", id, MessageType::INFO); 
      lcd.popAndDisplay();

      uint32_t mqttTimer = mqtt.getSettings().timer * 1e3;
      if (mqttTimer != lastTimerValue){
        updateInfluxInterval(mqttTimer); 
      }
      
      uint32_t sleepTimeMs = lastTimerValue;
      if (sleepTimeMs > 3*lcdInterval){
        sleepTimeMs -= 3*lcdInterval;
      } else{
        sleepTimeMs = 10;
      }

      manageSleepTime(sleepTimeMs); // Entra in sleep e al risveglio rimetterà flagWrite = true
    }
    else
    {
      // Se c'è un qualunque problema (allarme attivo O invio Influx fallito),
      // gestisce l'allarme visivo. Il Ticker checkAlarmStatus riattiverà flagCheckSensor tra 5 secondi.
      alarm.nextAlarm();
    }
  } // Chiusura corretta di if (flagCheckSensor)
  keepButtonAlive();
}

void keepButtonAlive(){
  int reading = digitalRead(RESET_ALARMS);
  if (reading != lastButtonState)
  {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE_DELAY)
  {
    static bool wasAlreadyPressed = false;
    if (reading == LOW && !wasAlreadyPressed)
    {
      alarm.setAllAlarmAcked();
      wasAlreadyPressed = true;
    }
    if (reading == HIGH)
    {
      wasAlreadyPressed = false;
    }
  }
  lastButtonState = reading;
}
void wakeupCallback()
{ // unlike ISRs, you can do a print() from a callback function
  Serial.println(F("Woke from Light Sleep - this is the callback"));
  Serial.flush();
}

void manageSleepTime(uint32_t sleepTimeMs)
{
  tickerAlarm.detach();
  checkAlarmStatus.detach();
  Serial.flush();
  // carichiamo i millisecondi effettivi che vogliamo dormire in questo ciclo.
  uint32_t remainingSleepTime = sleepTimeMs;



  Serial.print(F("CPU going to sleep for "));
  Serial.print(sleepTimeMs);
  Serial.println(F(" ms..."));
  Serial.flush();
  delay(100);

  wifi_set_opmode_current(NULL_MODE);
  yield();
  delay(50);
  extern os_timer_t *timer_list;
  timer_list = nullptr; // ferma i 4 timer del sistema operativo per consentire lo sleep

  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  wifi_fpm_open();
  wifi_fpm_set_wakeup_cb(wakeupCallback);

  // dall'hardware (maxTimerValue), facciamo dormire il chip a "blocchi" di durata massima.
  while (remainingSleepTime > maxTimerValue)
  {
    wifi_fpm_do_sleep(maxTimerValue * 1000); // Vuole i microsecondi (ms * 1000)
    esp_delay(maxTimerValue + 1);            // Il delay deve essere di 1ms superiore allo sleep
    remainingSleepTime -= maxTimerValue;     // Sottraiamo il blocco appena dormito
  }

  // Non ha senso mandare l'ESP in light sleep per un tempo inferiore a 10 ms (limite hardware)
  if (remainingSleepTime >= 10)
  {
    wifi_fpm_do_sleep(remainingSleepTime * 1000); // Usa il tempo rimanente effettivo!
    esp_delay(remainingSleepTime + 1);
  }

  // --- OPERAZIONI DI RISVEGLIO ---
  tickerAlarm.attach(1.5, []() {
    alarm.nextAlarm();
  });

  checkAlarmStatus.attach(5.0, [](){
    flagCheckSensor = true;
  });

  resetConnection();
  mqtt.sendWakeupStatus();

  unsigned long startMqttWindow = millis();
  while (millis() - startMqttWindow < 2000) {
    mqtt.handle();
    keepButtonAlive(); // Manteniamo comunque il pulsante reattivo
    yield();
  }

  flagWrite = true;       // Autorizziamo UN NUOVO invio pulito a InfluxDB per questo risveglio
  flagCheckSensor = true; // Forziamo l'aggiornamento immediato dei sensori senza attendere i 5s del Ticker
}

void resetConnection() {
  Serial.println(F("\n--- RISVEGLIO: Ripristino dello stack di rete ---"));
  
  // 1. Riaccendiamo la radio Wi-Fi in modalità Station
  wifi_set_opmode(STATION_MODE);
  wifi_station_connect();

  Serial.print(F("Connessione al Wi-Fi in corso"));
  
  // Attendiamo la connessione fisica al router (max 10 secondi)
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
    yield();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("\n[Wi-Fi] Connesso con successo!"));

    // 2. Re-inizializziamo e connettiamo MQTT
    Serial.println(F("[MQTT] Riconnessione al broker..."));
    mqtt.begin(client, "broker.emqx.io", 1883);
    
    // Forziamo un ciclo di handle per avviare la connessione MQTT
    mqtt.handle(); 
  } else {
    Serial.println(F("\n[Wi-Fi] Errore: Timeout connessione fallita al risveglio."));
  }
}

bool updateInfluxInterval(uint32_t newIntervalSeconds){
  if (newIntervalSeconds <=0){
    return false;
  }

  Serial.print(F("Aggiornamento intervallo InfluxDB: "));
  Serial.print(newIntervalSeconds);
  Serial.println(F(" secondi"));

  lastTimerValue = newIntervalSeconds;

  return true;
}
