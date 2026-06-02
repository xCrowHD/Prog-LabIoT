#include <iostream>
#include <algorithm>
#include <cstdint>
#include <fstream> 
#include <stdlib.h>
#include <stdio.h>

#include "../GreenhouseESP/AlarmHandler.h"
#include "../GreenhouseESP/InfluxHandler.h"
#include "../GreenhouseESP/LCDHandler.h"
#include "../GreenhouseESP/MqttHandler.h"
#include "../GreenhouseESP/SensorManager.h"
#include "../GreenhouseESP/HandleExceptions.h"

#include <iostream>
#include <string>

inline int testsFailed = 0;
inline int totalAssertions = 0;

#define EXPECT(condizione, messaggio)                                                                         \
    do                                                                                                        \
    {                                                                                                         \
        totalAssertions++;                                                                                    \
        if (!(condizione))                                                                                    \
        {                                                                                                     \
            testsFailed++;                                                                                    \
            std::cout << "\n[FALLITO] Assertion mancata alla riga " << __LINE__ << " di " << __FILE__ << "\n" \
                      << "  -> Condizione: " << #condizione << "\n"                                           \
                      << "  -> Messaggio:  " << messaggio << "\n\n";                                          \
        }                                                                                                     \
    } while (0)

#define RSSI_THRESHOLD -80
long rssi;
const char* url = "influx url";
const char* org = "organization";
const char* bkt = "Personal bucket";
const char* tkn = "Your token";

AlarmHandler alarm({.testMode = true});
LCDHandler lcd({.testMode = true});
InfluxHandler client(url, org, bkt, tkn);
HandleExceptions checkStatus(alarm, lcd, client);


bool mockMqttStatus;
InfluxStatus mockInfluxStatus;

void loopSimulation(PlantData &data, Thresholds &currentThr, InfluxStatus status);
void initializeMockStatuses(const std::string& testName = "");

void testAlarmHandler()
{
    initializeMockStatuses("Test 1");

    // Test 1: Aggiunta di un allarme
    std::cout << "Test 1: Aggiunta di un allarme" << std::endl;
    alarm.addAlarm(AlarmType::SENSOR_ERROR);
    EXPECT(checkStatus.isExecutionAllowed() == true,
        "L'allarme dovrebbe essere attivo dopo l'aggiunta di un allarme.");

    initializeMockStatuses("Test 2");
    // Test 2: Rimozione di un allarme
    std::cout << "Test 2: Rimozione di un allarme" << std::endl;
    alarm.removeAlarm(AlarmType::SENSOR_ERROR);
    EXPECT(alarm.getActiveAlarms().empty() == true,
        "L'elenco degli allarmi attivi dovrebbe essere vuoto dopo la rimozione di un allarme.");

    initializeMockStatuses("Test 3");
    // Test 3: Aggiunta di più allarmi
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

    // Test 4: Verifica dello stato dell'allarme
    initializeMockStatuses("Test 4");
    std::cout << "Test 4: Verifica dello stato dell'allarme" << std::endl;
    EXPECT(checkStatus.isExecutionAllowed() == true,
        "L'allarme dovrebbe essere attivo quando ci sono allarmi attivi.");

    // Test 5: Disabilitazione dell'allarme
    initializeMockStatuses("Test 5");
    std::cout << "Test 5: Disabilitazione dell'allarme" << std::endl;
    checkStatus.flipEnabled();
    EXPECT(checkStatus.isExecutionAllowed() == false,
        "L'allarme dovrebbe essere disabilitato dopo la chiamata a flipEnabled.");

    {// Test 6: loop di visualizzazione degli allarmi
    initializeMockStatuses("Test 6");
    std::cout << "Test 6: loop di visualizzazione degli allarmi" << std::endl << std::endl;
    alarm.addAlarm(AlarmType::SENSOR_ERROR);
    alarm.addAlarm(AlarmType::CONNECTION_ERROR);
    alarm.addAlarm(AlarmType::INFLUX_ERROR);
    // Simuliamo più cicli di visualizzazione per vedere il cambio di colore
    for (int i = 0; i < 4; ++i) {
        alarm.nextAlarmColor();
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
        alarm.nextAlarmColor();
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
        alarm.nextAlarmColor();
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
        alarm.nextAlarmColor();
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
        alarm.nextAlarmColor();
        
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
        alarm.nextAlarmColor();
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
        alarm.nextAlarmColor();
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
            alarm.nextAlarmColor();
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
        alarm.nextAlarmColor();
    }

    std::cout << "Simulazione ciclo con connessione WiFi disattivata completata." << std::endl;
    rssi = -60;// Riattiviamo la connessione per i test successivi
    loopSimulation(validData, validThresholds, InfluxStatus::SUCCESS); // Simuliamo un ciclo con connessione attiva per vedere il cambio di stato
    
    for (size_t i = 0; i < alarm.getActiveAlarms().size(); ++i)
    {
        std::cout << "Active Alarm " << i + 1 << ": ";
        alarm.nextAlarmColor();
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
        alarm.nextAlarmColor();

        std::cout << "Simulazione ciclo di guarigione con dati validi..." << std::endl;
        PlantData validData = {25.0, 50.0, 300, true};
        loopSimulation(validData, validThresholds, InfluxStatus::SUCCESS);

        // Chiamata diretta: il set è vuoto, nextAlarmColor() intercetta la salute e stampa VERDE
        std::cout << "Stato LED dopo guarigione: ";
        alarm.nextAlarmColor();

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
        alarm.nextAlarmColor();
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
            alarm.nextAlarmColor();
        }

        std::cout << "Risoluzione dell'errore INFLUX_ERROR..." << std::endl;
        loopSimulation(validData, someOutOfRangeThresholds, InfluxStatus::SUCCESS);

        auto alarmsPhase2 = alarm.getActiveAlarms();

        for (size_t i = 0; i < alarmsPhase2.size(); ++i)
        {
            std::cout << "Active Alarm " << i + 1 << ": ";
            alarm.nextAlarmColor();
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
            alarm.nextAlarmColor();
        }

        EXPECT(alarmsPhase3.size() == 0, "Dovrebbe esserci 0 allarmi attivi (ALL_OK) dopo la risoluzione di tutti gli errori.");
    }

    { // Test 18: aggiunta nuovi allarmi mentre disabilitato.
        initializeMockStatuses("Test 18");
        std::cout << "Test 18: Alarm disable. Mi aspetto che il sistema raccolga nuovi dati allarmi mentre disabilitato \n"
                    "ma che non siano visualizzati mentre disattivo"
                    << std::endl << std::endl;

        std::cout << "Simuliamo SENSOR ERROR" << std::endl << std::endl;
        alarm.addAlarm(AlarmType::SENSOR_ERROR);
        alarm.nextAlarmColor();

        // Verifichiamo che lo stato iniziale sia attivo e l'allarme sia dentro
        EXPECT(checkStatus.isExecutionAllowed() == true, "L'allarme dovrebbe essere abilitato all'inizio.");
        EXPECT(alarm.getActiveAlarms().size() == 1, "Dovrebbe esserci 1 allarme nel vettore.");

        // Disattiviamo il sistema
        std::cout << "\nDisattiviamo il sistema" << std::endl;
        checkStatus.flipEnabled();
        EXPECT(checkStatus.isExecutionAllowed() == false, "L'allarme dovrebbe essere disattivato dopo il flip.");

        std::cout << "\nSimuliamo 3 errori mentre disabilitato" << std::endl
                      << std::endl;
        alarm.addAlarm(AlarmType::SOME_THRESHOLDS_OUT);
        alarm.addAlarm(AlarmType::CONNECTION_ERROR);
        alarm.addAlarm(AlarmType::INFLUX_ERROR);

        // Proviamo a ciclare i LED: non dovrebbe mostrare nulla/andare in ledOff() internamente
        std::cout << "Colori visualizzati mentre disattivo:" << std::endl;
        alarm.nextAlarmColor();

        // Controlliamo che il vettore si sia riempito lo stesso (Vecchio tolto + 3 nuovi = 3)
        auto alarmPhase1 = alarm.getActiveAlarms();
        std::cout << "Dimensione vettore mentre disabilitato: " << alarmPhase1.size() << std::endl;
        EXPECT(alarmPhase1.size() == 3, "Il vettore dovrebbe contenere 3 allarmi anche se disattivato.");

        // Riattiviamo il sistema
        std::cout << "\nRiattiviamo il sistema" << std::endl;
        checkStatus.flipEnabled();
        EXPECT(checkStatus.isExecutionAllowed() == true, "L'allarme dovrebbe essere nuovamente attivo dopo il secondo flip.");

        // Verifichiamo che la sequenza non sia andata persa dopo la riattivazione
        auto alarmPhase2 = alarm.getActiveAlarms();
        EXPECT(alarmPhase2.size() == 3, "Il vettore deve mantenere i 3 allarmi dopo essere stato riattivato.");

        std::cout << "\nVisualizziamo la sequenza di allarmi generati:" << std::endl;
        for (size_t i = 0; i < alarmPhase2.size(); i++)
        {
                alarm.nextAlarmColor();
        }
    }
    { // Test 19: Disabilitazione temporizzata
        initializeMockStatuses("Test 19");
        std::cout << "Test 19: Disabilitazione temporizzata" << std::endl;
        std::cout << "\ndisableTime impostato a 2 millisecondi" << std::endl;
        checkStatus.getConfig().disableTime = 2;

        std::cout << "Disabilitazione allarme (tempo 0)" << std::endl;
        checkStatus.flipEnabled(); // Viene registrato il timestamp attuale (es. 0)

        // Fase 1: Il tempo è a 0, l'allarme viene bloccato

        std::cout << "Aggiungiamo SENSOR ERROR" << std::endl;
        PlantData invalidData = {25.0, 50.0, 300, false}; // Dati non validi
        Thresholds validThresholds = {"Tomato", 20.0, 30.0, 40.0, 60.0, 200, 400};
        checkStatus.handleDataException(invalidData); // Simuliamo un ciclo con dati non validi per attivare l'allarme SENSOR_ERROR
        
        //alarm.addAlarm(AlarmType::SENSOR_ERROR);
        std::cout << "nextAlarmColor()...";
        alarm.nextAlarmColor();
        std::cout << "lcd.popAndDisplay()...\n";
        lcd.popAndDisplay();
        std::cout << std::endl;

        auto lcdQueue = lcd.getQueue();
        EXPECT(lcdQueue.empty(), "LCD dovrebbe essere vuoto perché l'allarme è bloccato");
        
        std::cout << "Numero allarmi attivi: " << alarm.getActiveAlarms().size() << std::endl;
        EXPECT(alarm.getActiveAlarms().empty() == true, "Alarm non deve registrare questo evento");

        std::cout << "\nAvanziamo il tempo di 1ms" << std::endl;
        delay(1);

        // Siamo a 1ms (minore di 2ms): ancora bloccato
        std::cout << "Aggiungiamo CONNECTION ERROR. L'evento non deve essere registrato" << std::endl;
        checkStatus.handleConnectionException(RSSI_THRESHOLD - 20, RSSI_THRESHOLD); 
        std::cout << "nextAlarmColor()...";
        alarm.nextAlarmColor();
        std::cout << "lcd.popAndDisplay()...\n";
        lcd.popAndDisplay();
        std::cout << std::endl;

        auto lcdQueue2 = lcd.getQueue();
        EXPECT(lcdQueue2.empty(), "LCD dovrebbe essere vuoto perché l'allarme è ancora bloccato");
        
        std::cout << "Numero allarmi attivi: " << alarm.getActiveAlarms().size() << std::endl;
        EXPECT(alarm.getActiveAlarms().empty(), "Alarm non deve registrare questo evento");

        // Avanziamo di un altro millisecondo (Totale tempo = 2ms)
        std::cout << "\nAvanziamo di un ulteriore ms (tempo = 2ms)" << std::endl;
        delay(1);

        // Fase 2: Il tempo di blocco è scaduto, l'allarme viene accettato in background
        std::cout << "Aggiungiamo ancora CONNECTION_ERROR" << std::endl;
        checkStatus.handleConnectionException(RSSI_THRESHOLD - 20, RSSI_THRESHOLD);
        std::cout << "Verifichiamo che il sistema ha accettato l'allarme" << std::endl;
        std::cout << "nextAlarmColor()...";
        alarm.nextAlarmColor();
        std::cout << "lcd.popAndDisplay()...\n";
        lcd.popAndDisplay();
        std::cout << std::endl;

        auto lcdQueue3 = lcd.getQueue();
        EXPECT(lcdQueue3.size() == 1, "LCD dovrebbe avere un messaggio in coda perché l'allarme è stato accettato dopo la scadenza del blocco");
        
        std::cout << "Numero allarmi attivi: " << alarm.getActiveAlarms().size() << std::endl;
        EXPECT(alarm.getActiveAlarms().size() == 1, "Alarm deve registare questo evento anche se disabilitato");

        std::cout << "\nRiattiviamo l'allarme" << std::endl;
        checkStatus.flipEnabled();
        std::cout << "nextAlarmcolor()...";
        alarm.nextAlarmColor();
        std::cout << "lcd.popAndDisplay()...\n";
        lcd.popAndDisplay();
        std::cout << std::endl;
        EXPECT(alarm.getActiveAlarms().size() == 1, "Allarme cancellato erroneamente");
    }
}

int main()
{
    // Apre il file sovrascrivendo eventuali esecuzioni precedenti

    std::cout << "=======================================" << std::endl;
    std::cout << "              SUITE TEST               " << std::endl;
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
        std::cout << "[ATTENSIONE] Rilevati " << std::to_string(testsFailed) << " fallimenti su " << std::to_string(totalAssertions) << " verifiche totali" << std::endl << std::endl;
        return 1;
    }
}
//-------------------------------------- Funzioni di gestione delle eccezioni per i test ---------------------------------------//
void initializeMockStatuses(const std::string& testName)
{
    
    std::cout << std::endl << "---Inizializzando Mock Status per il test " << testName << "---" << std::endl;

    checkStatus.getConfig().disableTime = 0; // Imposto un tempo di disabilitazione breve per i test
    checkStatus.getConfig().testMode = true;  // Abilito i messaggi di debug per i LED su console
    alarm.getConfig().testMode = true;
    lcd.begin();
    
    if (!checkStatus.isExecutionAllowed())
    {
        checkStatus.flipEnabled(); 
    }
    alarm.clearAlarms();

    rssi = -60;
    mockMqttStatus = true;
    mockInfluxStatus = InfluxStatus::SUCCESS;
}

void loopSimulation(PlantData &data, Thresholds &currentThr, InfluxStatus status)
{
    bool connStatus = checkStatus.handleConnectionException(rssi, RSSI_THRESHOLD);
    bool dataStatus = checkStatus.handleDataException(data);
    bool mqttStatus = checkStatus.handleMqttExceptions(currentThr);
    bool thrStatus = checkStatus.handleThresholds(data, currentThr);

    if (client.isReadyToWrite() && connStatus && dataStatus)
    {
        status = client.sendDataToInflux(data, rssi, "Serra", "NodeMCU", currentThr);
    }

    bool influxStatus = checkStatus.handleInfluxException(status);

    // Valutazione dello stato globale per aggiornare scritte LCD di successo
    if (connStatus && dataStatus && mqttStatus && thrStatus && influxStatus)
    {
        checkStatus.handleSuccess();
    }
    else
    {
        alarm.nextAlarmColor();
    }
}