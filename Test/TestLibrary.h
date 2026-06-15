#ifndef TEST_LIBRARY_H
#define TEST_LIBRARY_H

#include <iostream>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "../GreenhouseESP/AlarmHandler.h"
#include "../GreenhouseESP/InfluxHandler.h"
#include "../GreenhouseESP/LCDHandler.h"
#include "../GreenhouseESP/MqttHandler.h"
#include "../GreenhouseESP/SensorManager.h"
#include "../GreenhouseESP/HandleExceptions.h"

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

#endif // TEST_LIBRARY_H