/**
 * =========================================================
 *  Chaleco Inteligente – ESP32 Dev
 * =========================================================
 *  Sensores:
 *    HC-SR04  (ultrasonidos)  TRIG=18  ECHO=19
 *    DHT11    (temp/humedad)  DATA=13
 *    Semáforo LED             VERDE=25  AMARILLO=26  ROJO=27
 *    Buzzer activo THDZ       GPIO14
 *
 *  Lógica de distancia:
 *    > 30 cm  → LED verde  + silencio
 *    10-30 cm → LED amarillo + 2 pitidos/segundo
 *    < 10 cm  → LED rojo  + 5 pitidos/segundo
 *
 *  Por consola (115200 baud) cada 2 s:
 *    Distancia, temperatura y humedad
 * =========================================================
 */

#include <Arduino.h>
#include <DHT.h>

// ── Pines ────────────────────────────────────────────────
#define PIN_GREEN    25
#define PIN_YELLOW   26
#define PIN_RED      27

#define PIN_TRIG     18
#define PIN_ECHO     19

#define PIN_DHT      13
#define DHT_TYPE     DHT11

#define PIN_BUZZER   14

// ── Umbrales de distancia (cm) ────────────────────────────
#define DIST_FAR     30
#define DIST_NEAR    10

// ── Buzzer ────────────────────────────────────────────────
#define BEEP_MS        10   // duración de cada pitido (ms)

// ── Objetos ───────────────────────────────────────────────
DHT dht(PIN_DHT, DHT_TYPE);

// ── Variables globales ────────────────────────────────────
float    g_distance  = 0.0f;
float    g_tempC     = 0.0f;
float    g_humidity  = 0.0f;

unsigned long g_lastDistMs   = 0;
unsigned long g_lastDHTMs    = 0;
unsigned long g_lastBuzzerMs = 0;
bool          g_buzzerOn     = false;

// ── Prototipos ────────────────────────────────────────────
float measureDistance();
void  updateLEDs(float dist);
void  updateBuzzer(float dist);
void  readDHT();
void  printSerial();

// =========================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  // LEDs semáforo
  pinMode(PIN_GREEN,  OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_RED,    OUTPUT);
  digitalWrite(PIN_GREEN,  LOW);
  digitalWrite(PIN_YELLOW, LOW);
  digitalWrite(PIN_RED,    LOW);

  // HC-SR04
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  // DHT11
  dht.begin();

  // Buzzer (aseguramos que esté en silencio al inicio)
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  Serial.println();
  Serial.println(F("==========================================="));
  Serial.println(F("  Chaleco Inteligente – ESP32 iniciado"));
  Serial.println(F("  HC-SR04 | DHT11 | Semaforo | Buzzer activo"));
  Serial.println(F("==========================================="));
}

// =========================================================
void loop() {
  unsigned long now = millis();

  // Medir distancia cada 100 ms
  if (now - g_lastDistMs >= 100UL) {
    g_lastDistMs = now;
    g_distance   = measureDistance();
    updateLEDs(g_distance);
  }

  // Leer DHT11 y publicar por consola cada 2 s
  if (now - g_lastDHTMs >= 2000UL) {
    g_lastDHTMs = now;
    readDHT();
    printSerial();
  }

  // Actualizar buzzer de forma no bloqueante
  updateBuzzer(g_distance);
}

// =========================================================
//  HC-SR04: retorna distancia en cm (999 si fuera de rango)
// =========================================================
float measureDistance() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // Timeout ~30 ms equivale a ~5 m máximo
  long duration = pulseIn(PIN_ECHO, HIGH, 30000UL);
  if (duration == 0) return 999.0f;

  return duration * 0.034f / 2.0f;  // cm
}

// =========================================================
//  Semáforo: apaga los tres LEDs y enciende el que corresponda
// =========================================================
void updateLEDs(float dist) {
  digitalWrite(PIN_GREEN,  LOW);
  digitalWrite(PIN_YELLOW, LOW);
  digitalWrite(PIN_RED,    LOW);

  if (dist > DIST_FAR) {
    digitalWrite(PIN_GREEN,  HIGH);
  } else if (dist >= DIST_NEAR) {
    digitalWrite(PIN_YELLOW, HIGH);
  } else {
    digitalWrite(PIN_RED,    HIGH);
  }
}

// =========================================================
//  Buzzer activo – gestión no bloqueante con digitalWrite
//
//  > 30 cm  → silencio
//  10-30 cm → período  500 ms (2 pitidos/s)
//  < 10 cm  → período  200 ms (5 pitidos/s)
// =========================================================
void updateBuzzer(float dist) {
  // Silencio total cuando la distancia es superior a 30 cm
  if (dist > DIST_FAR) {
    if (g_buzzerOn) {
      digitalWrite(PIN_BUZZER, LOW);
      g_buzzerOn = false;
    }
    return;
  }

  unsigned long period = (dist >= DIST_NEAR) ? 500UL : 200UL;
  unsigned long elapsed = millis() - g_lastBuzzerMs;

  if (!g_buzzerOn && elapsed >= period) {
    // Iniciar pitido
    digitalWrite(PIN_BUZZER, HIGH);
    g_buzzerOn     = true;
    g_lastBuzzerMs = millis();
  } else if (g_buzzerOn && elapsed >= BEEP_MS) {
    // Detener pitido
    digitalWrite(PIN_BUZZER, LOW);
    g_buzzerOn = false;
  }
}

// =========================================================
//  DHT11: lee temperatura y humedad (guarda último valor válido)
// =========================================================
void readDHT() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(h)) g_humidity = h;
  if (!isnan(t)) g_tempC    = t;
}

// =========================================================
//  Salida por consola
// =========================================================
void printSerial() {
  const char* estado;
  if (g_distance > DIST_FAR) {
    estado = "VERDE   (> 30 cm)  - sin pitido";
  } else if (g_distance >= DIST_NEAR) {
    estado = "AMARILLO (10-30 cm) - 2 pitidos/s";
  } else {
    estado = "ROJO    (< 10 cm)  - 5 pitidos/s";
  }

  Serial.println(F("-------------------------------------------"));
  Serial.printf( "  Distancia  : %.1f cm\n",   g_distance);
  Serial.printf( "  Temperatura: %.1f °C\n",   g_tempC);
  Serial.printf( "  Humedad    : %.1f %%\n",   g_humidity);
  Serial.printf( "  Semaforo   : %s\n",        estado);
  Serial.println(F("-------------------------------------------"));
}