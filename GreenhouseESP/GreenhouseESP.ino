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

volatile bool flagAwakeFromBtn = false;
unsigned long timeStampStartSleeping = 0;
uint32_t totalSleepTime = 0;


void manageSleepTime(uint32_t sleepTimeMs);
bool updateInfluxInterval(uint32_t newIntervalSeconds);
void keepButtonAlive();

// Funzione di interrupt (ISR) posizionata in IRAM
void IRAM_ATTR manageButtonInterrupt() {
  gpio_pin_wakeup_disable();
  flagAwakeFromBtn = true;
}

void setup()
{
  Serial.begin(115200);
  WiFiHandler::begin();
  WiFiHandler::getMacAddress(id);
  pinMode(RESET_ALARMS, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(RESET_ALARMS), manageButtonInterrupt, FALLING);

  mqtt.begin(client, "broker.emqx.io", 1883);
  lcd.begin();
  sensor.begin();
  alarm.begin();

  checkAlarmStatus.attach(5.0, [](){
    flagCheckSensor = true;
  });

  flagWrite = true;
}

void loop()
{
  mqtt.handle();
  

  if (millis() - lastLcdUpdate >= lcdInterval){
    lastLcdUpdate = millis();
    lcd.addMessage("ID", id, MessageType::INFO); 
    lcd.popAndDisplay(); 
    alarm.nextAlarm();
  }

  if (mqtt.isStandBy()){
    lcd.addMessage("Status", "StandByMode", MessageType::INFO);
    return;
  }

  if (!mqtt.isSet()){
    alarm.addAlarm(AlarmType::NEED_SETTINGS);
    return;
  }
  alarm.removeAlarm(AlarmType::NEED_SETTINGS);

  if(!mqtt.isRunning()){
   lcd.addMessage("Status", "Offline", MessageType::INFO);
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

      if (connStatus && dataStatus){
        InfluxStatus status = client_idb.sendDataToInflux(data, rssi, "Serra", "NodeMCU", currentThr);
        influxStatus = checkStatus.handleInfluxException(status);
      }
      else{
        influxStatus = false;
      }


      if (influxStatus){
        // SUCCESSO: Abbassiamo il flag così NON ci riproverà più fino al prossimo risveglio
        flagWrite = false;
      }
    }
    // ---------------------------------
    
    // Valutazione dello stato globale per l'ingresso in Light Sleep
    std::deque<LCDMsg> lcdQueue = lcd.getQueue();

    bool hasWarnings = false;
    bool hasErrors = false;

    std::vector<AlarmType> activeAlarms = alarm.getActiveAlarms();

    for (const auto &alarmType : activeAlarms){
      MessageType type = alarm.getAlarmMessage(alarmType).type;

      if (type == MessageType::ERROR)
        hasErrors = true;

      if (type == MessageType::WARNING)
        hasWarnings = true;
    }

    if (!hasErrors) // Se non ha errori può entrare in deepsleep
    {
      if (!hasWarnings) // Se non ha neanche warnings lampeggerà di verde, altrimenti dei colori corretti
        checkStatus.handleSuccess();
        

      lcd.addMessage("System", "Going to sleep", MessageType::INFO);
      lcd.popAndDisplay();

      mqtt.sendSleepingStatus();

      // -- CICLO DI SVUOTAMENTO PRIMA DELLO SLEEP ---
      // Manteniamo LCD attivo fintanto che non ha mostrato tutti i messaggi una volta
      // anche nel caso di successo immediato dei booleani nel loop


      unsigned long int lastMessageTime = millis();
      int que_lenght = lcd.getQueue().size();

      while (millis() - lastMessageTime < que_lenght*lcdInterval){ //mostra l'ultimo ciclo di messaggi
        if (millis() - lastLcdUpdate >= lcdInterval){
          lastLcdUpdate = millis();
          lcd.popAndDisplay();
          alarm.nextAlarm();
        }
        delay(1);
        keepButtonAlive();
        yield();
      }

      // --- CONFIGURAZIONE DISPLAY PER IL PERIODO DI SLEEP ---
      // La coda è stata mostrata. Adesso stampiamo sul display SOLO l'ID dell'ESP.
      // Questo messaggio rimarrà impresso staticamente sul display per tutta la durata del sonno.
      lcd.clearAll();                               // Se il tuo handler ha un clear, pulisce lo schermo

      lcd.addMessage("Sleep Mode", id, MessageType::INFO); 
      lcd.popAndDisplay();

      uint32_t mqttTimer = mqtt.getSettings().timer * 1e3;
      if (mqttTimer != lastTimerValue){
        updateInfluxInterval(mqttTimer); 
      }
      
      uint32_t sleepTimeMs = lastTimerValue;
      if (sleepTimeMs > que_lenght*lcdInterval){
        sleepTimeMs -= que_lenght*lcdInterval;
      } else{
        sleepTimeMs = 10;
      }

      totalSleepTime = sleepTimeMs;
      timeStampStartSleeping = millis();
      manageSleepTime(sleepTimeMs); // Entra in sleep e al risveglio rimetterà flagWrite = true
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

  // Se è passato il tempo di debounce, lo stato è considerato stabile
  if ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE_DELAY)
  {
    static int lastStableState = reading; 

    if (reading != lastStableState)
    {
      alarm.setAllAlarmAcked();
      lastStableState = reading;
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
  Serial.flush();
  uint32_t remainingSleepTime = sleepTimeMs;

  if (totalSleepTime == 0) {
    totalSleepTime = sleepTimeMs;
    timeStampStartSleeping = millis();
  }

  checkAlarmStatus.detach();

  Serial.print(F("CPU going to sleep for "));
  Serial.print(sleepTimeMs);
  Serial.println(F(" ms..."));
  Serial.flush();
  delay(100);

  wifi_set_opmode_current(NULL_MODE);
  yield();
  delay(50);
  
  extern os_timer_t *timer_list;
  os_timer_t *old_timer_list = timer_list;
  timer_list = nullptr; 

  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  wifi_fpm_open();
  wifi_fpm_set_wakeup_cb(wakeupCallback);

  detachInterrupt(digitalPinToInterrupt(RESET_ALARMS));
  gpio_pin_wakeup_enable(RESET_ALARMS, GPIO_PIN_INTR_LOLEVEL);
  flagAwakeFromBtn = false;

  // Ciclo di sleep a blocchi hardware
  while (remainingSleepTime > maxTimerValue && !flagAwakeFromBtn)
  {
    wifi_fpm_do_sleep(maxTimerValue * 1000); 
    delay(maxTimerValue + 1);            
    remainingSleepTime -= maxTimerValue; 
  }

  if (remainingSleepTime >= 10 && !flagAwakeFromBtn)
  {
    wifi_fpm_do_sleep(remainingSleepTime * 1000); 
    delay(remainingSleepTime + 1);
  }

  // --- OPERAZIONI IMMEDIATE POST-RISVEGLIO (STRATO BASSO) ---
  gpio_pin_wakeup_disable();
  timer_list = old_timer_list;
  
  // Ripristiniamo subito l'interrupt standard di Arduino sul pin per i click a CPU sveglia
  attachInterrupt(digitalPinToInterrupt(RESET_ALARMS), manageButtonInterrupt, FALLING);

  // Controllo se il risveglio è stato causato dal pulsante
  if (flagAwakeFromBtn) {
    flagAwakeFromBtn = false;
    
    Serial.println(F("[WAKE] Svegliato anzitempo dal pulsante! Esecuzione ACK..."));
    
    // Esegui l'ACK (aggiorna lo stato interno dell'AlarmHandler e spegne/cambia i LED)
    alarm.setAllAlarmAcked(); 

    // Calcoliamo quanto tempo reale è rimasto per completare lo sleep globale originale
    unsigned long currentSleepTime = millis() - timeStampStartSleeping;

    if (currentSleepTime < totalSleepTime) {
      uint32_t remaingSleepTime = totalSleepTime - currentSleepTime;
      
      if (remaingSleepTime > 100) {
        Serial.println(F("[WAKE] ACK eseguito. Torno subito in Light Sleep..."));
        Serial.flush();
        
        // RICORSIONE: Si rimette a dormire.
        manageSleepTime(remaingSleepTime); 
        
        // CRITICO: Quando la chiamata sopra terminerà (sonno finito), questa istanza 
        // deve interrompersi immediatamente con un 'return' per evitare di scendere 
        // ed eseguire il risveglio completo una seconda volta.
        return; 
      }
    }
  }

  // --- RISVEGLIO EFFETTIVO COMPLETATO ---
  // Questo blocco viene eseguito SOLO dall'istanza principale quando il timer originario è scaduto del tutto
  totalSleepTime = 0;
  timeStampStartSleeping = 0;

  Serial.println(F("[SYSTEM] Tempo totale di sleep esaurito. Avvio periferiche e invio dati..."));

  checkAlarmStatus.attach(5.0, [](){
    flagCheckSensor = true;
  });

  resetConnection();
  mqtt.sendWakeupStatus();

  unsigned long startMqttWindow = millis();
  while (millis() - startMqttWindow < 2000) {
    mqtt.handle();
    keepButtonAlive(); 
    lcd.addMessage("Status", "Waking up", MessageType::INFO);
    lcd.popAndDisplay();
    yield();
  }

  flagWrite = true;       // Autorizza l'invio a InfluxDB solo ora che lo sleep è finito del tutto
  flagCheckSensor = true; // Forza il controllo dei sensori
}

void resetConnection() {
  Serial.println(F("\n--- RISVEGLIO: Ripristino dello stack di rete ---"));
  
  // 1. Riaccendiamo la radio Wi-Fi in modalità Station
  wifi_set_opmode(STATION_MODE);
  wifi_station_connect();

  Serial.print(F("Connessione al Wi-Fi in corso"));
  
  unsigned long startWifiTimeout = millis();
  // Attendiamo la connessione fisica al router (max 10 secondi)
  while (WiFi.status() != WL_CONNECTED && millis() - startWifiTimeout < 10000) {
    keepButtonAlive();
    delay(50);
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
