#include <DHT.h>
#include <Ticker.h>

// DHT sensor
#define DHTPIN D1     // sensor I/O pin, eg. D1 (DO NOT USE D0 or D4! see above notes)
#define DHTTYPE DHT11  // sensor type DHT 11

void readTempFunc();
Ticker readDHTtick;


bool readTemp = false;
DHT dht = DHT(DHTPIN, DHTTYPE);

void setup() {
  dht.begin();
  Serial.begin(115200);
  readDHTtick.attach(2.0, readTempFunc);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(readTemp)
  {
    readTemp = false;
    readDHT();
  }
}

void readDHT()
{
  float h = dht.readHumidity();      // humidity percentage, range 20-80% (±5% accuracy)
  float t = dht.readTemperature();   // temperature Celsius, range 0-50°C (±2°C accuracy)
  if (isnan(h) || isnan(t)) 
  {   // readings failed, skip
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
}

void readTempFunc()
{
  readTemp = true;
}
