#include "MqttHandler.h"


#define COLLISION D1
#define FLAME_DIG D2
#define NTC_PIN A0  // NTC analog pin
#define NTC_R1 10000
#define BUZZER D3
#define FIRE_THR 30.0

// Steinhart-Hart coefficients for the NTC
#define NTC_A 3.354016e-03
#define NTC_B 2.569850e-04
#define NTC_C 2.620131e-06
#define NTC_D 6.383091e-08

bool isDoorClose = false;
bool isOnFlame = false;
unsigned long ultimoControlloSicurezza = 0;

WiFiClient client;
MqttHandler mqtt;

void setup() {
  Serial.begin(115200);
  WiFiHandler::begin();

  mqtt.begin(client, "broker.emqx.io", 1883);
  // put your setup code here, to run once:
  pinMode(COLLISION, INPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, HIGH);
  pinMode(FLAME_DIG, INPUT);
  pinMode(FLAME_DIG, INPUT_PULLUP);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Sincronizzazione orario Unix");
  while (time(nullptr) < 10000) {  // Aspetta che il timestamp diventi un numero valido
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nOrario sincronizzato!");
  Serial.println(F("\n\nSetup completed.\n\n"));
}

void loop() {
  mqtt.handle();
  if (millis() - ultimoControlloSicurezza >= 500) {
    ultimoControlloSicurezza = millis();

    checkCollisioneState();  // Controlla la porta
    checkIncendioState();    // Controlla l'incendio (Fiamma + Temp)
  }
}

bool detectCollision() {
  byte val = digitalRead(COLLISION);  // read the collision state

  if (val == LOW) {
    // collision detected!
    return true;
  }
  return false;
}

float getTemperature() {
  // read ADC and compute the temperature, see page 4 of the datasheet
  float Vo = analogRead(NTC_PIN);             // voltage, range 0~1023
  float R2 = NTC_R1 * ((float)Vo / 1023.0f);  // compute the resistance on thermistor at current temperature
  float logR2R1 = log(R2 / NTC_R1);
  float T = 1.0f / (NTC_A + (NTC_B * logR2R1) + (NTC_C * (logR2R1 * logR2R1)) + (NTC_D * (logR2R1 * logR2R1 * logR2R1)));  // temperature in Kelvin
  T = T - 273.15f;                                                                                                         // Kelvin to Celsius
  return T;
}

void turnBuzzerOn() {
  digitalWrite(BUZZER, LOW);
}

bool detectFlames() {
  bool digitalVal = digitalRead(FLAME_DIG);
  if (digitalVal == HIGH) {
    return true;
  }
  return false;
}

void checkCollisioneState() {
  bool statoAttuale = detectCollision();
  if (statoAttuale != isDoorClose) {

    isDoorClose = statoAttuale;

    if (isDoorClose == true) {
      Serial.println(F("Collisione Rilevata!"));
      mqtt.sendDoorPayload(true);
    } else {
      Serial.println(F("Collisione Risolta / Stato Ripristinato!"));
      mqtt.sendDoorPayload(false);
    }
  }
}

void checkIncendioState() {
  bool fiammaRilevata = detectFlames();
  float tempAttuale = getTemperature();
  bool statoAttualeIncendio = (fiammaRilevata && tempAttuale > FIRE_THR);

  if (statoAttualeIncendio != isOnFlame) {
    isOnFlame = statoAttualeIncendio;

    if (isOnFlame == true) {
      Serial.println(F("[ALLARME] Incendio Rilevato! Fiamma attiva e temperatura critica!"));
      turnBuzzerOn();
      mqtt.sendFlamePayload(true, tempAttuale)
    } else {
      Serial.println(F("[RIPRISTINO] Emergenza rientrata o falso allarme terminato."));
      digitalWrite(BUZZER, HIGH);  // Spegne il buzzer

      mqtt.sendFlamePayload(false, tempAttuale)
    }
  }
}