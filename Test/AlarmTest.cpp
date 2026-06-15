#include "TestLibrary.h"

#define RSSI_THRESHOLD -80
#define RESET_ALARMS 5
#define BUTTON_DEBOUNCE_DELAY 20 

unsigned long lastTimerValue;
long rssi;
const char* url = "influx url";
const char* org = "organization";
const char* bkt = "Personal bucket";
const char* tkn = "Your token";

unsigned long lastDebounceTime = 0; // L'ultima volta che il pin è stato campionato
bool lastButtonState = HIGH;

char id[13];

volatile bool flagCheckSensor;
volatile bool flagWrite;

unsigned long lastLcdUpdate = 0;        // ms
const unsigned long lcdInterval = 2000; // ms

LCDHandler lcd;

LED alarmLed = {9, 10, 11}; // Pin RGB LED
AlarmHandler alarm(lcd, alarmLed);

InfluxHandler client(url, org, bkt, tkn);
HandleExceptions checkStatus(alarm, client);


bool mockMqttStatus;
InfluxStatus mockInfluxStatus;

void loopSimulation(PlantData &data, Thresholds &currentThr, InfluxStatus status);
void initializeMockStatuses(const std::string& testName = "");
void keepButtonAlive();
void manageSleepTime(uint32_t sleepTimeMs);

void testAlarmHandler()
{
    {//Test 1: Aggiunta di un allarme
    initializeMockStatuses("Test 1");

    // Test 1: Aggiunta di un allarme
    std::cout << "Test 1: Aggiunta di un allarme" << std::endl;
    alarm.addAlarm(AlarmType::SENSOR_ERROR);
    EXPECT(!alarm.getConfig().ack,
        "L'allarme dovrebbe essere attivo dopo l'aggiunta di un allarme.");
    }
    {//Test 2: Rimozione di un allarme
    initializeMockStatuses("Test 2");
    // Test 2: Rimozione di un allarme
    std::cout << "Test 2: Rimozione di un allarme" << std::endl;
    alarm.removeAlarm(AlarmType::SENSOR_ERROR);
    EXPECT(alarm.getActiveAlarms().empty() == true,
        "L'elenco degli allarmi attivi dovrebbe essere vuoto dopo la rimozione di un allarme.");
    }
    {//Test 3: Aggiunta di più allarmi
    initializeMockStatuses("Test 3");
    std::cout << "Test 3: Aggiunta di più allarmi" << std::endl;
    alarm.addAlarm(AlarmType::SOME_THRESHOLDS_OUT);
    alarm.addAlarm(AlarmType::ALL_THRESHOLDS_OUT);
    EXPECT(alarm.getActiveAlarms().size() == 2,
        "Dovrebbero esserci 2 allarmi attivi.");
    EXPECT(std::find(alarm.getActiveAlarms().begin(), 
        alarm.getActiveAlarms().end(), 
        AlarmType::SOME_THRESHOLDS_OUT) != alarm.getActiveAlarms().end(),
         "L'allarme SOME_THRESHOLDS_OUT dovrebbe essere presente.");

    EXPECT(std::find(alarm.getActiveAlarms().begin(), 
        alarm.getActiveAlarms().end(), 
        AlarmType::ALL_THRESHOLDS_OUT) != alarm.getActiveAlarms().end(),
         "L'allarme ALL_THRESHOLDS_OUT dovrebbe essere presente.");
    }
    {// Test 4: Verifica dello stato dell'allarme
    initializeMockStatuses("Test 4");
    std::cout << "Test 4: Verifica dello stato dell'allarme" << std::endl;
    EXPECT(alarm.getConfig().ack == false,
        "L'allarme dovrebbe essere attivo quando ci sono allarmi attivi.");
    }

    {// Test 6: loop di visualizzazione degli allarmi
    initializeMockStatuses("Test 6");
    std::cout << "Test 6: loop di visualizzazione degli allarmi" << std::endl << std::endl;
    alarm.addAlarm(AlarmType::SENSOR_ERROR);
    alarm.addAlarm(AlarmType::CONNECTION_ERROR);
    alarm.addAlarm(AlarmType::INFLUX_ERROR);
    // Simuliamo più cicli di visualizzazione per vedere il cambio di colore
    for (int i = 0; i < 4; ++i) {
        alarm.nextAlarm();
    }
    }   

    {// Test 7: comportamento del main loop con vari scenari
    initializeMockStatuses("Test 7");
    std::cout << "Test 7: WiFi connection error" << std::endl;
    
    rssi = -100;
    PlantData validData = {25.0, 50.0, 300, true};
    Thresholds validThresholds = {"Tomato", 20.0, 30.0, 40.0, 60.0, 200, 400};

    loopSimulation(validData, validThresholds, mockInfluxStatus);

    auto alarmPhase = alarm.getActiveAlarms();

    for (size_t i = 0; i < alarmPhase.size(); ++i)
    {
        std::cout << "Active Alarm " << i + 1 << ": ";
        alarm.nextAlarm();
    }
    EXPECT(std::find(alarmPhase.begin(), alarmPhase.end(), AlarmType::CONNECTION_ERROR) != alarmPhase.end(),
           "L'allarme CONNECTION_ERROR dovrebbe essere presente.");
    EXPECT(std::find(alarmPhase.begin(), alarmPhase.end(),
        AlarmType::CONNECTION_ERROR) != alarmPhase.end(),
        "L'allarme CONNECTION_ERROR dovrebbe essere presente.");

    }

    //-----------------------------------------------------------------------------------

    {// Test 8: sensor data non validi, tutto il resto valido
    initializeMockStatuses("Test 8");
    std::cout << "Test 8: sensor data invalid" << std::endl << std::endl;

    Thresholds validThresholds = {"Tomato", 20.0, 30.0, 40.0, 60.0, 200, 400};
    PlantData invalidData = {25.0, 50.0, 300, false}; // Dati non validi
    loopSimulation(invalidData, validThresholds, mockInfluxStatus);

    for (size_t i = 0; i < alarm.getActiveAlarms().size(); ++i)
    {
        std::cout << "Active Alarm " << i + 1 << ": ";
        alarm.nextAlarm();
        lcd.popAndDisplay();
    }

    EXPECT(alarm.getActiveAlarms().size() == 1,
        "Dovrebbe esserci 1 allarme attivo (SENSOR_ERROR).");
    EXPECT(std::find(alarm.getActiveAlarms().begin(),
        alarm.getActiveAlarms().end(),
        AlarmType::SENSOR_ERROR) != alarm.getActiveAlarms().end(),
        "L'allarme SENSOR_ERROR dovrebbe essere presente.");
    }

    {// Test 9: Influx connection error
    initializeMockStatuses("Test 9");
    std::cout << "Test 9: Influx connection error" << std::endl << std::endl;

    mockInfluxStatus = InfluxStatus::ERR_INFLUX_CONNECTION;
    PlantData validData = {25.0, 50.0, 300, true};
    Thresholds validThresholds = {"Tomato", 20.0, 30.0, 40.0, 60.0, 200, 400};
    loopSimulation(validData, validThresholds, mockInfluxStatus);
    for (size_t i = 0; i < alarm.getActiveAlarms().size(); ++i)
    {
        std::cout << "Active Alarm " << i + 1 << ": ";
        alarm.nextAlarm();
        lcd.popAndDisplay();
    }
    EXPECT(alarm.getActiveAlarms().size() == 1,
        "Dovrebbe esserci 1 allarme attivo (INFLUX_ERROR).");
    EXPECT(std::find(alarm.getActiveAlarms().begin(),
        alarm.getActiveAlarms().end(),
        AlarmType::INFLUX_ERROR) != alarm.getActiveAlarms().end(),
        "L'allarme INFLUX_ERROR dovrebbe essere presente.");

    }
    
    {// Test 10: Mqtt exception (missing plant name)
    initializeMockStatuses("Test 10");
    std::cout << "Test 10: Mqtt exception (missing plant name)" << std::endl << std::endl;
    PlantData validData = {25.0, 50.0, 300, true};
    Thresholds invalidThresholds = {"", 20.0, 30.0, 40.0, 60.0, 200, 400}; // Nome pianta mancante
    loopSimulation(validData, invalidThresholds, mockInfluxStatus);
    for (size_t i = 0; i < alarm.getActiveAlarms().size(); ++i)
    {
        std::cout << "Active Alarm " << i + 1 << ": ";
        alarm.nextAlarm();
        
    }
    EXPECT(alarm.getActiveAlarms().size() == 1,
        "Dovrebbe esserci 1 allarme attivo (NO_SEND_DATA).");
    EXPECT(std::find(alarm.getActiveAlarms().begin(),
        alarm.getActiveAlarms().end(), AlarmType::NO_SEND_DATA) != alarm.getActiveAlarms().end(),
        "L'allarme NO_SEND_DATA dovrebbe essere presente.");

    }

    {
    // Test 11:Thresholds out of range
    initializeMockStatuses("Test 11");
    std::cout << "Test 11: All thresholds out of range" << std::endl << std::endl;
    PlantData validData = {25.0, 50.0, 300, true};
    Thresholds outOfRangeThresholds = {"Tomato", 10.0, 20.0, 30.0, 40.0, 100, 200}; // Tutte le soglie fuori range
    loopSimulation(validData, outOfRangeThresholds, mockInfluxStatus);
    for (size_t i = 0; i < alarm.getActiveAlarms().size(); ++i)
    {
        std::cout << "Active Alarm " << i + 1 << ": ";
        alarm.nextAlarm();
    }
    EXPECT(alarm.getActiveAlarms().size() == 1,
        "Dovrebbe esserci 1 allarme attivo (ALL_THRESHOLDS_OUT).");
    EXPECT(std::find(alarm.getActiveAlarms().begin(),
        alarm.getActiveAlarms().end(), AlarmType::ALL_THRESHOLDS_OUT) != alarm.getActiveAlarms().end(),
        "L'allarme ALL_THRESHOLDS_OUT dovrebbe essere presente.");
    }

    {
    // Test 12: Some thresholds out of range
    initializeMockStatuses("Test 12");
    std::cout << "Test 12: Some thresholds out of range" << std::endl << std::endl;
    PlantData validData = {25.0, 50.0, 300, true};
    Thresholds someOutOfRangeThresholds = {"Tomato", 20.0, 30.0, 30.0, 40.0, 100, 200}; // Solo la temperatura è fuori range
    loopSimulation(validData, someOutOfRangeThresholds, mockInfluxStatus);
    for (size_t i = 0; i < alarm.getActiveAlarms().size(); ++i)
    {
        std::cout << "Active Alarm " << i + 1 << ": ";
        alarm.nextAlarm();
    }
    EXPECT(alarm.getActiveAlarms().size() == 1,
        "Dovrebbe esserci 1 allarme attivo (SOME_THRESHOLDS_OUT).");
    EXPECT(std::find(alarm.getActiveAlarms().begin(),
        alarm.getActiveAlarms().end(), AlarmType::SOME_THRESHOLDS_OUT) != alarm.getActiveAlarms().end(),
        "L'allarme SOME_THRESHOLDS_OUT dovrebbe essere presente.");

    }

    {// Test 13: Tutto OK
        initializeMockStatuses("Test 13");
        std::cout << "Test 13: All systems operational" << std::endl << std::endl;
        PlantData validData = {25.0, 50.0, 300, true};
        Thresholds validThresholds = {"Tomato", 20.0, 30.0, 40.0, 60.0, 200, 400};
        loopSimulation(validData, validThresholds, InfluxStatus::SUCCESS);
        for (size_t i = 0; i < alarm.getActiveAlarms().size(); ++i)
        {
            std::cout << "Active Alarm " << i + 1 << ": ";
            alarm.nextAlarm();
        }
        EXPECT(alarm.getActiveAlarms().size() == 0,
            "Dovrebbe esserci 0 allarmi attivi (ALL_OK)."); 

    }

    {// Test 14: Loop connessione WiFi disattivata e tutto il resto valido
    initializeMockStatuses("Test 14");
    std::cout << "Test 14: Loop connessione WiFi disattivata e tutto il resto valido" << std::endl << std::endl;
    PlantData validData = {25.0, 50.0, 300, true};
    Thresholds validThresholds = {"Tomato", 20.0, 30.0, 40.0, 60.0, 200, 400};
    
    std::cout << "Simulazione ciclo con connessione WiFi disattivata..." << std::endl;
    rssi = -100;
    loopSimulation(validData, validThresholds, InfluxStatus::SUCCESS);
    for (size_t i = 0; i < alarm.getActiveAlarms().size(); ++i)
    {
        std::cout << "Active Alarm " << i + 1 << ": ";
        alarm.nextAlarm();
    }

    std::cout << "Simulazione ciclo con connessione WiFi disattivata completata." << std::endl;
    rssi = -60;// Riattiviamo la connessione per i test successivi
    loopSimulation(validData, validThresholds, InfluxStatus::SUCCESS); // Simuliamo un ciclo con connessione attiva per vedere il cambio di stato
    
    for (size_t i = 0; i < alarm.getActiveAlarms().size(); ++i)
    {
        std::cout << "Active Alarm " << i + 1 << ": ";
        alarm.nextAlarm();
    }
    EXPECT(alarm.getActiveAlarms().size() == 0,
        "Dovrebbe esserci 0 allarmi attivi (ALL_OK) dopo la riattivazione della connessione.");
    }

    { // Test 15: Transizione errore sensore -> risoluzione
        initializeMockStatuses("Test 15");

        std::cout << "Simulazione ciclo con dati non validi..." << std::endl << std::endl;
        PlantData invalidData = {25.0, 50.0, 300, false};
        Thresholds validThresholds = {"Tomato", 20.0, 30.0, 40.0, 60.0, 200, 400};
        loopSimulation(invalidData, validThresholds, InfluxStatus::SUCCESS);

        // Qui il set ha dimensione 1, quindi chiamiamo nextAlarmColor direttamente
        std::cout << "Stato LED: ";
        alarm.nextAlarm();

        std::cout << "Simulazione ciclo di guarigione con dati validi..." << std::endl;
        PlantData validData = {25.0, 50.0, 300, true};
        loopSimulation(validData, validThresholds, InfluxStatus::SUCCESS);

        // Chiamata diretta: il set è vuoto, nextAlarmColor() intercetta la salute e stampa VERDE
        std::cout << "Stato LED dopo guarigione: ";
        alarm.nextAlarm();

        // Verifica logica rigorosa
        EXPECT(alarm.getActiveAlarms().empty() == true,
               "BUG: Il set degli allarmi dovrebbe essere vuoto dopo la guarigione!");
    }

    {// Test 16: Loop con MQTT exception (missing plant name) e tutto il resto valido
    initializeMockStatuses("Test 16");
    std::cout << "Test 16: Loop con MQTT exception (missing plant name) e tutto il resto valido" << std::endl << std::endl;
    PlantData validData = {25.0, 50.0, 300, true};
    Thresholds invalidThresholds = {"", 20.0, 30.0, 40.0, 60.0, 200, 400}; // Nome pianta mancante
    loopSimulation(validData, invalidThresholds, InfluxStatus::SUCCESS);    
    Thresholds validThresholds = {"Tomato", 20.0, 30.0, 40.0, 60.0, 200, 400}; // Nome pianta valido
    loopSimulation(validData, validThresholds, InfluxStatus::SUCCESS); // Simuliamo un ciclo con nome pianta valido per vedere il cambio di stato
    for (size_t i = 0; i < alarm.getActiveAlarms().size(); ++i)
    {
        std::cout << "Active Alarm " << i + 1 << ": ";
        alarm.nextAlarm();
    }
    }

    { // Test 17: loop di 2 errori (SOME_THRESHOLDS_OUT + INFLUX_ERROR) e poi risoluzione di entrambi
        initializeMockStatuses("Test 17");
        std::cout << "Test 17: loop di 2 errori (SOME_THRESHOLDS_OUT + INFLUX_ERROR) e poi risoluzione di entrambi" << std::endl << std::endl;

        PlantData validData = {25.0, 50.0, 300, true};
        Thresholds someOutOfRangeThresholds = {"Tomato", 20.0, 30.0, 30.0, 40.0, 100, 200};

        // 1. Attiviamo entrambi gli errori
        loopSimulation(validData, someOutOfRangeThresholds, InfluxStatus::ERR_INFLUX_CONNECTION);

        // Verifichiamo lo stato IMMEDIATAMENTE prima di stampare o muovere iteratori
        auto alarmsPhase1 = alarm.getActiveAlarms();
        EXPECT(alarmsPhase1.size() == 2, "Dovrebbero esserci 2 allarmi attivi.");

        for (size_t i = 0; i < alarmsPhase1.size(); ++i)
        
        {
            std::cout << "Active Alarm " << i + 1 << ": ";
            alarm.nextAlarm();
        }

        std::cout << "Risoluzione dell'errore INFLUX_ERROR..." << std::endl;
        loopSimulation(validData, someOutOfRangeThresholds, InfluxStatus::SUCCESS);

        auto alarmsPhase2 = alarm.getActiveAlarms();

        for (size_t i = 0; i < alarmsPhase2.size(); ++i)
        {
            std::cout << "Active Alarm " << i + 1 << ": ";
            alarm.nextAlarm();
        }

        bool trovatoInflux = (std::find(alarmsPhase2.begin(), alarmsPhase2.end(), AlarmType::INFLUX_ERROR) != alarmsPhase2.end());
        bool trovatoSoglie = (std::find(alarmsPhase2.begin(), alarmsPhase2.end(), AlarmType::SOME_THRESHOLDS_OUT) != alarmsPhase2.end());

        EXPECT(!trovatoInflux, "BUG: L'allarme INFLUX_ERROR e' ancora nel set!");
        EXPECT(trovatoSoglie, "BUG: L'allarme SOME_THRESHOLDS_OUT dovrebbe essere rimasto!");
        EXPECT(alarmsPhase2.size() == 1, "BUG: Il conteggio totale degli allarmi non e' 1!");

        std::cout << "Risoluzione dell'errore SOME_THRESHOLDS_OUT..." << std::endl;
        Thresholds validThresholds = {"Tomato", 20.0, 30.0, 40.0, 60.0, 200, 400};
        loopSimulation(validData, validThresholds, InfluxStatus::SUCCESS);
        auto alarmsPhase3 = alarm.getActiveAlarms();

        for (size_t i = 0; i < alarmsPhase3.size(); ++i)
        {
            std::cout << "Active Alarm " << i + 1 << ": ";
            alarm.nextAlarm();
        }

        EXPECT(alarmsPhase3.size() == 0, "Dovrebbe esserci 0 allarmi attivi (ALL_OK) dopo la risoluzione di tutti gli errori.");
    }

    { // Test 18 gestione disable/enable degli allarmi
    initializeMockStatuses("Test 18");
    std::cout << "Test 18: gestione disable/enable degli allarmi" << std::endl << std::endl;
    std::cout << "SENSOR ERROR " << std::endl;
    PlantData invalidData = {25.0, 50.0, 300, false}; // Dati non validi
    Thresholds validThresholds = {"Tomato", 20.0, 30.0, 40.0, 60.0, 200, 400};
    loopSimulation(invalidData, validThresholds, InfluxStatus::SUCCESS);
    std::cout << "\nAggiungiamo insuccesso influx..." << std::endl;
    loopSimulation(invalidData, validThresholds, InfluxStatus::ERR_INFLUX_CONNECTION);
    alarm.nextAlarm();
    lcd.popAndDisplay();

    std::cout << "Prendiamo nota degli allarmi..." << std::endl;
    alarm.setAllAlarmAcked();
    loopSimulation(invalidData, validThresholds, InfluxStatus::ERR_INFLUX_CONNECTION);
    alarm.nextAlarm();
    lcd.popAndDisplay();
    lcd.popAndDisplay();

    std::cout << "\nSparisce SENSOR ERROR" << std::endl;
    PlantData validData = {25.0, 50.0, 300, true}; // Dati validi
    loopSimulation(validData, validThresholds, InfluxStatus::ERR_INFLUX_CONNECTION);
    alarm.nextAlarm();
    lcd.popAndDisplay();

    std::cout << "\nTorna SENSOR ERROR" << std::endl;
    loopSimulation(invalidData, validThresholds, InfluxStatus::ERR_INFLUX_CONNECTION);
    alarm.nextAlarm();
    lcd.popAndDisplay();
}
}

int main()
{
    // Apre il file sovrascrivendo eventuali esecuzioni precedenti

    std::cout << "=======================================" << std::endl;
    std::cout << "              ALARM TEST               " << std::endl;
    std::cout << "=======================================" << std::endl;

    testAlarmHandler();

    std::cout << "=======================================" << std::endl;
    std::cout << "             RESOCONTO FINALE          " << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << "Controlli totali eseguiti: " << std::to_string(totalAssertions) << std::endl;

    if (testsFailed == 0)
    {
        std::cout << "[SUCCESSO] Tutti i test sono passati senza errori! " << std::endl << std::endl;
        return 0;
    }
    else
    {
        std::cout << "[ATTENZIONE] Rilevati " << std::to_string(testsFailed) << " fallimenti su " << std::to_string(totalAssertions) << " verifiche totali" << std::endl << std::endl;
        return 1;
    }
}
//-------------------------------------- Funzioni di gestione delle eccezioni per i test ---------------------------------------//
void initializeMockStatuses(const std::string& testName)
{
    
    std::cout << std::endl << "---Inizializzando Mock Status per il test " << testName << "---" << std::endl;

    alarm.getConfig().testMode = true;
    alarm.getConfig().ack = false;
    flagWrite = true;

    lcd.begin();
    alarm.clearAlarms();

    rssi = -60;
    mockMqttStatus = true;
    mockInfluxStatus = InfluxStatus::SUCCESS;
}

void loopSimulation(PlantData &data, Thresholds &currentThr, InfluxStatus status)
{
        // I singoli metodi aggiungono o rimuovono gli allarmi in autonomia
        bool connStatus = checkStatus.handleConnectionException(rssi, RSSI_THRESHOLD);
        bool dataStatus = checkStatus.handleDataException(data);
        bool mqttStatus = checkStatus.handleMqttExceptions(currentThr);
        bool thrStatus = checkStatus.handleThresholds(data, currentThr);

        if (dataStatus)
        {
            lcd.addMessagePlantData(data.temperature, data.humidity, data.light);
        }

        // Logica risveglio e invio dati a Influx ESATTAMENTE 1 volta mentre è sveglio
        bool influxStatus = true;

        if (flagWrite)
        {
            /*InfluxStatus status = InfluxStatus::SUCCESS;

            if (connStatus && dataStatus)
            {
                status = client.sendDataToInflux(data, rssi, "Serra", "NodeMCU", currentThr);
            }
            else
            {
                status = InfluxStatus::ERR_INFLUX_CONNECTION;
            }*/

            influxStatus = checkStatus.handleInfluxException(status);

            if (influxStatus)
            {
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

            // -- CICLO DI SVUOTAMENTO PRIMA DELLO SLEEP ---
            // Manteniamo LCD attivo fintanto che non ha mostrato tutti i messaggi una volta
            // anche nel caso di successo immediato dei booleani nel loop

            unsigned long int lastMessageTime = millis();

            while (millis() - lastMessageTime < 3 * lcdInterval)
            { // mostra gli ultimi 3 messaggi
                if (millis() - lastLcdUpdate >= lcdInterval)
                {
                    lastLcdUpdate = millis();
                    lcd.popAndDisplay();
                }
                delay(10);
                keepButtonAlive();
            }

            // --- CONFIGURAZIONE DISPLAY PER IL PERIODO DI SLEEP ---
            // La coda è stata mostrata. Adesso stampiamo sul display SOLO l'ID dell'ESP.
            // Questo messaggio rimarrà impresso staticamente sul display per tutta la durata del sonno.
            lcd.clearAll(); // Se il tuo handler ha un clear, pulisce lo schermo
            lcd.addMessage("ID", id, MessageType::INFO);
            lcd.popAndDisplay();

            uint32_t sleepTimeMs = lastTimerValue * 1000;
            if (sleepTimeMs > 3 * lcdInterval)
            {
                sleepTimeMs -= 3 * lcdInterval;
            }
            else
            {
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

void keepButtonAlive()
{
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
}

void manageSleepTime(uint32_t sleepTimeMs)
{
    delay(10);
}

bool updateInfluxInterval(uint32_t newIntervalSeconds)
{
    if (newIntervalSeconds <= 0)
    {
        return false;
    }

    Serial.print(F("Aggiornamento intervallo InfluxDB: "));
    Serial.print(newIntervalSeconds);
    Serial.println(F(" secondi"));

    lastTimerValue = newIntervalSeconds;

    return true;
}
