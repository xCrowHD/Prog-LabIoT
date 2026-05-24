#include <iostream>
#include <algorithm>
#include <cstdint>

#include <stdlib.h>
#include <stdio.h>

#include "AlarmHandler.h"
#include "InfluxHandler.h"
#include "MqttHandler.h"

#include <iostream>
#include <string>

inline int testsFailed = 0;
inline int totalAssertions = 0;

// La nostra macro personalizzata: non fa crashare il programma ma logga l'errore
#define EXPECT(condizione, messaggio)                                                                         \
    do                                                                                                        \
    {                                                                                                         \
        totalAssertions++;                                                                                    \
        if (!(condizione))                                                                                    \
        {                                                                                                     \
            testsFailed++;                                                                                    \
            std::cerr << "\n[FALLITO] Assertion mancata alla riga " << __LINE__ << " di " << __FILE__ << "\n" \
                      << "  -> Condizione: " << #condizione << "\n"                                           \
                      << "  -> Messaggio:  " << messaggio << "\n\n";                                          \
        }                                                                                                     \
    } while (0)

AlarmHandler alarm;
bool mockConnStatus;
bool mockMqttStatus;
InfluxStatus mockInfluxStatus;

bool handleMqttExceptions(Thresholds &currentThr);
bool handleThresholds(PlantData &data, Thresholds &currentThr);
bool handleInfluxException(InfluxStatus status);
bool handleDataException(PlantData &data);
bool handleConnectionException();
void handleSuccess();
void loopSimulation(PlantData &data, Thresholds &currentThr, InfluxStatus status);
void initializeMockStatuses(const std::string& testName = "");

void testAlarmHandler()
{
    /*initializeMockStatuses();

    // Test 1: Aggiunta di un allarme
    std::cout << "Test 1: Aggiunta di un allarme" << std::endl;
    alarm.addAlarm(AlarmType::SENSOR_ERROR);
    assert(alarm.getAlarmStatus() == true &&
        "L'allarme dovrebbe essere attivo dopo l'aggiunta di un allarme.");

    initializeMockStatuses();
    // Test 2: Rimozione di un allarme
    std::cout << "Test 2: Rimozione di un allarme" << std::endl;
    alarm.removeAlarm(AlarmType::SENSOR_ERROR);
    assert(alarm.getActiveAlarms().empty() == true && 
        "L'elenco degli allarmi attivi dovrebbe essere vuoto dopo la rimozione di un allarme.");

    initializeMockStatuses();
    // Test 3: Aggiunta di più allarmi
    std::cout << "Test 3: Aggiunta di più allarmi" << std::endl;
    alarm.addAlarm(AlarmType::SOME_THRESHOLDS_OUT);
    alarm.addAlarm(AlarmType::ALL_THRESHOLDS_OUT);
    assert(alarm.getActiveAlarms().size() == 2 && 
        "Dovrebbero esserci 2 allarmi attivi.");
    assert(std::find(alarm.getActiveAlarms().begin(), 
        alarm.getActiveAlarms().end(), 
        AlarmType::SOME_THRESHOLDS_OUT) != alarm.getActiveAlarms().end() &&
         "L'allarme SOME_THRESHOLDS_OUT dovrebbe essere presente.");

    assert(std::find(alarm.getActiveAlarms().begin(), 
        alarm.getActiveAlarms().end(), 
        AlarmType::ALL_THRESHOLDS_OUT) != alarm.getActiveAlarms().end() &&
         "L'allarme ALL_THRESHOLDS_OUT dovrebbe essere presente.");

    // Test 4: Verifica dello stato dell'allarme
    initializeMockStatuses();
    std::cout << "Test 4: Verifica dello stato dell'allarme" << std::endl;
    assert(alarm.getAlarmStatus() == true &&
        "L'allarme dovrebbe essere attivo quando ci sono allarmi attivi.");

    // Test 5: Disabilitazione dell'allarme
    initializeMockStatuses();
    std::cout << "Test 5: Disabilitazione dell'allarme" << std::endl;
    alarm.flipEnabled();
    assert(alarm.getAlarmStatus() == false &&
        "L'allarme dovrebbe essere disabilitato dopo la chiamata a flipEnabled.");

    */
    {// Test 6: loop di visualizzazione degli allarmi
    initializeMockStatuses("Test 6");
    std::cout << "Test 6: loop di visualizzazione degli allarmi" << std::endl;
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
    
    mockConnStatus = false;
    PlantData validData = {25.0, 50.0, 300, true};
    Thresholds validThresholds = {"Tomato", 20.0, 30.0, 40.0, 60.0, 200, 400};

    loopSimulation(validData, validThresholds, mockInfluxStatus);

    for (size_t i = 0; i < alarm.getActiveAlarms().size(); ++i)
    {
        std::cout << "Active Alarm " << i + 1 << ": ";
        alarm.nextAlarmColor();
    }
    EXPECT(alarm.getActiveAlarms().size() == 1,
        "Dovrebbe esserci 1 allarme attivo (CONNECTION_ERROR).");
    EXPECT(std::find(alarm.getActiveAlarms().begin(),
        alarm.getActiveAlarms().end(),
        AlarmType::CONNECTION_ERROR) != alarm.getActiveAlarms().end(),
        "L'allarme CONNECTION_ERROR dovrebbe essere presente.");

    }

    //-----------------------------------------------------------------------------------

    {// Test 8: sensor data non validi, tutto il resto valido
    initializeMockStatuses("Test 8");
    std::cout << "Test 8: sensor data invalid" << std::endl;

    mockConnStatus = true;
    Thresholds validThresholds = {"Tomato", 20.0, 30.0, 40.0, 60.0, 200, 400};
    PlantData invalidData = {25.0, 50.0, 300, false}; // Dati non validi
    loopSimulation(invalidData, validThresholds, mockInfluxStatus);

    for (size_t i = 0; i < alarm.getActiveAlarms().size(); ++i)
    {
        std::cout << "Active Alarm " << i + 1 << ": ";
        alarm.nextAlarmColor();
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
    std::cout << "Test 9: Influx connection error" << std::endl;

    mockInfluxStatus = InfluxStatus::ERR_INFLUX_CONNECTION;
    PlantData validData = {25.0, 50.0, 300, true};
    Thresholds validThresholds = {"Tomato", 20.0, 30.0, 40.0, 60.0, 200, 400};
    loopSimulation(validData, validThresholds, mockInfluxStatus);
    for (size_t i = 0; i < alarm.getActiveAlarms().size(); ++i)
    {
        std::cout << "Active Alarm " << i + 1 << ": ";
        alarm.nextAlarmColor();
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
    std::cout << "Test 10: Mqtt exception (missing plant name)" << std::endl;
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
    std::cout << "Test 11: All thresholds out of range" << std::endl;
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
    std::cout << "Test 12: Some thresholds out of range" << std::endl;
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
        std::cout << "Test 13: All systems operational" << std::endl;
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
    std::cout << "Test 14: Loop connessione WiFi disattivata e tutto il resto valido" << std::endl;
    PlantData validData = {25.0, 50.0, 300, true};
    Thresholds validThresholds = {"Tomato", 20.0, 30.0, 40.0, 60.0, 200, 400};
    
    std::cout << "Simulazione ciclo con connessione WiFi disattivata..." << std::endl;
    mockConnStatus = false;
    loopSimulation(validData, validThresholds, InfluxStatus::SUCCESS);
    for (size_t i = 0; i < alarm.getActiveAlarms().size(); ++i)
    {
        std::cout << "Active Alarm " << i + 1 << ": ";
        alarm.nextAlarmColor();
    }

    std::cout << "Simulazione ciclo con connessione WiFi disattivata completata." << std::endl;
    mockConnStatus = true; // Riattiviamo la connessione per i test successivi
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

        std::cout << "Simulazione ciclo con dati non validi..." << std::endl;
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
    std::cout << "Test 16: Loop con MQTT exception (missing plant name) e tutto il resto valido" << std::endl;
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
        std::cout << "Test 17: loop di 2 errori (SOME_THRESHOLDS_OUT + INFLUX_ERROR) e poi risoluzione di entrambi" << std::endl;

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

    {
        // Test
    }
}

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "       INIZIO DELLA SUITE DI TEST       " << std::endl;
    std::cout << "========================================" << std::endl;

    testAlarmHandler();

    std::cout << "========================================" << std::endl;
    std::cout << "             RESOCONTO FINALE           " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Controlli totali eseguiti: " << totalAssertions << std::endl;

    if (testsFailed == 0)
    {
        std::cout << "\n[SUCCESSO] Tutti i test sono passati senza errori!\n"
                  << std::endl;
        return 0;
    }
    else
    {
        std::cerr << "\n[ATTENZIONE] Rilevati " << testsFailed << " fallimenti su "
                  << totalAssertions << " verifiche totali.\n"
                  << std::endl;
        return 1;
    }
}

//-------------------------------------- Funzioni di gestione delle eccezioni per i test ---------------------------------------//
void initializeMockStatuses(const std::string& testName)
{
    std::cout << "\n--- Initializing mock statuses for " << testName << " ---\n";
    alarm.begin();

    if (!alarm.getAlarmStatus())
    {
        alarm.flipEnabled(); // Assicuriamoci che l'allarme sia abilitato all'inizio dei test
    }
    alarm.clearAlarms();

    mockConnStatus = true;
    mockMqttStatus = true;
    mockInfluxStatus = InfluxStatus::SUCCESS;
}

bool handleMqttExceptions(Thresholds &currentThr)
{
    if (currentThr.plantName == nullptr || currentThr.plantName[0] == '\0')
    {
        alarm.addAlarm(AlarmType::NO_SEND_DATA);
        std::cout << "MQTT exception: Missing plant name" << std::endl;
        return false;
    }
    alarm.removeAlarm(AlarmType::NO_SEND_DATA);
    return true;
}

bool handleThresholds(PlantData &data, Thresholds &currentThr)
{
    bool tempInRange = data.temperature >= currentThr.tempMin && data.temperature <= currentThr.tempMax;
    bool humInRange = data.humidity >= currentThr.humMin && data.humidity <= currentThr.humMax;
    bool luxInRange = data.light >= currentThr.luxMin && data.light <= currentThr.luxMax;

    if (!tempInRange && !humInRange && !luxInRange)
    {
        alarm.addAlarm(AlarmType::ALL_THRESHOLDS_OUT);
        std::cout << "Thresholds exception: All thresholds out of range" << std::endl;
        return false;
    }
    else if (!tempInRange || !humInRange || !luxInRange)
    {
        alarm.addAlarm(AlarmType::SOME_THRESHOLDS_OUT);
        std::cout << "Thresholds exception: Some thresholds out of range" << std::endl;
        return false;
    }

    alarm.removeAlarm(AlarmType::SOME_THRESHOLDS_OUT);
    alarm.removeAlarm(AlarmType::ALL_THRESHOLDS_OUT);
    return true;
}

bool handleInfluxException(InfluxStatus status)
{
    if (status == InfluxStatus::ERR_INFLUX_CONNECTION)
    {
        alarm.addAlarm(AlarmType::INFLUX_ERROR);
        std::cout << "Influx exception: Connection error" << std::endl;
        return false;
    }

    alarm.removeAlarm(AlarmType::INFLUX_ERROR);
    return true;
}

bool handleDataException(PlantData &data)
{

    if (!data.valid)
    {
        alarm.addAlarm(AlarmType::SENSOR_ERROR);
        std::cout << "Data exception: Invalid sensor data" << std::endl;
        return false;
    }
    alarm.removeAlarm(AlarmType::SENSOR_ERROR);
    return true;
}

bool handleConnectionException()
{
    if (!mockConnStatus)
    {
        alarm.addAlarm(AlarmType::CONNECTION_ERROR);
        return false;
    }
    return true;
}

void handleSuccess()
{
    alarm.clearAlarms();
    std::cout << "All systems operational" << std::endl;
}

void loopSimulation(PlantData &data, Thresholds &currentThr, InfluxStatus status){
    bool connStatus = handleConnectionException();
    bool dataStatus = handleDataException(data);
    bool mqttStatus = handleMqttExceptions(currentThr);
    bool thrStatus = true; // Di base assumiamo siano OK, cambieranno solo se controllati
    bool influxStatus = true;

    // Controlliamo le soglie solo se i dati del sensore sono effettivamente validi
    if (dataStatus)
    {
        thrStatus = handleThresholds(data, currentThr);

        // Controlliamo Influx solo se siamo online
        if (connStatus)
        {
            influxStatus = handleInfluxException(status);
        }
        else
        {
            // Se non c'è connessione, non possiamo testare Influx adesso.
            // Rimuoviamo il vecchio errore Influx per non bloccare la logica futura.
            alarm.removeAlarm(AlarmType::INFLUX_ERROR);
        }
    }
    else
    {
        // Se i dati del sensore non sono validi, non possiamo testare le soglie.
        alarm.removeAlarm(AlarmType::SOME_THRESHOLDS_OUT);
        alarm.removeAlarm(AlarmType::ALL_THRESHOLDS_OUT);
    }

    // Rimuoviamo l'ALL_OK preventivamente per aggiornarlo alla fine
    alarm.removeAlarm(AlarmType::ALL_OK);

    // SE tutti i report dei singoli handler dicono che è tutto a posto
    if (connStatus && dataStatus && mqttStatus && thrStatus && influxStatus)
    {
        handleSuccess();
    }
}