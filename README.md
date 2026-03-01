# Chaleco Inteligente – ESP32

Sistema embebido instalado sobre un chaleco que utiliza un sensor de ultrasonidos para medir la distancia a obstáculos, un semáforo LED para señalización visual, un buzzer activo para alertas sonoras y un sensor DHT11 para monitorizar la temperatura y humedad ambiental.

---

## Hardware utilizado

### Microcontrolador
| Componente | Modelo | Descripción |
|---|---|---|
| Placa | ESP32 Dev Module | Microcontrolador con WiFi/BT, 3.3 V lógica, 38/30 pines |

---

### Sensores y actuadores

#### 1. Sensor de distancia por ultrasonidos – HC-SR04

| Parámetro | Valor |
|---|---|
| Modelo | HC-SR04 |
| Tensión de operación | 5 V (tolerante 3.3 V en Echo) |
| Rango de detección | 2 cm – 400 cm |
| Resolución | ~3 mm |
| Frecuencia de ultrasonido | 40 kHz |
| Pin TRIG | GPIO 18 (salida) |
| Pin ECHO | GPIO 19 (entrada) |
| GND | GND |
| VCC | 5 V (VIN) |

**Funcionamiento:** El ESP32 envía un pulso de 10 µs al pin TRIG. El sensor emite 8 pulsos ultrasónicos a 40 kHz y activa el pin ECHO durante el tiempo que tarda el eco en regresar. La distancia se calcula como:

```
distancia (cm) = duración_echo (µs) × 0.034 / 2
```

---

#### 2. Sensor de temperatura y humedad – DHT11

| Parámetro | Valor |
|---|---|
| Modelo | DHT11 |
| Tensión de operación | 3.3 V – 5 V |
| Rango temperatura | 0 °C – 50 °C (±2 °C) |
| Rango humedad | 20 % – 80 % RH (±5 %) |
| Frecuencia de muestreo máxima | 1 Hz (una lectura cada segundo) |
| Protocolo | 1-Wire propietario |
| Pin DATA | GPIO 13 |
| GND | GND |
| VCC | 3.3 V |

> Se lee cada 2 segundos para respetar el tiempo mínimo entre lecturas del DHT11.

---

#### 3. Semáforo LED de 4 pines

| Parámetro | Valor |
|---|---|
| Tipo | Módulo semáforo LED (3 colores) |
| Pines | 4 (Verde, Amarillo, Rojo, GND) |
| Pin LED Verde | GPIO 25 |
| Pin LED Amarillo | GPIO 26 |
| Pin LED Rojo | GPIO 27 |
| GND | GND |
| Tensión de operación | 3.3 V (lógica ESP32 directa) |

> El módulo lleva resistencias limitadoras integradas. Cada pin de color se conecta directamente al GPIO del ESP32.

---

#### 4. Buzzer activo – THDZ

| Parámetro | Valor |
|---|---|
| Modelo | THDZ (buzzer activo) |
| Tipo | Activo (suena al aplicar tensión, sin señal de frecuencia) |
| Tensión de operación | 5 V |
| Pin positivo (+) | GPIO 14 |
| Pin negativo (-) | GND |

> Al ser un buzzer **activo**, basta con poner el pin a `HIGH` para que suene y a `LOW` para silenciarlo. No requiere `tone()` ni ninguna señal de frecuencia externa.

---

## Conexiones – Resumen

```
ESP32 Dev          Componente
─────────────────────────────────────────
GPIO 18  ──────→  HC-SR04  TRIG
GPIO 19  ←──────  HC-SR04  ECHO
GPIO 13  ←──────  DHT11    DATA
GPIO 25  ──────→  Semáforo VERDE
GPIO 26  ──────→  Semáforo AMARILLO
GPIO 27  ──────→  Semáforo ROJO
GPIO 14  ──────→  Buzzer   (+)
GND      ──────→  HC-SR04 GND | DHT11 GND | Semáforo GND | Buzzer (-)
3.3V     ──────→  DHT11 VCC | Semáforo VCC
5V/VIN   ──────→  HC-SR04 VCC
```

---

## Funcionamiento del firmware

### Diagrama de estados por distancia

```
Distancia medida
       │
       ├─ > 30 cm  ──→  LED VERDE    + Buzzer SILENCIO
       │
       ├─ 10–30 cm ──→  LED AMARILLO + Buzzer 2 pitidos/segundo
       │
       └─ < 10 cm  ──→  LED ROJO     + Buzzer 5 pitidos/segundo
```

> Cuando la distancia supera los 30 cm el buzzer se apaga inmediatamente aunque estuviera sonando.

### Frecuencia de actualización

| Tarea | Intervalo |
|---|---|
| Medición HC-SR04 | Cada 100 ms |
| Control semáforo | Cada 100 ms (junto a medición) |
| Control buzzer | Cada iteración del loop (no bloqueante) |
| Lectura DHT11 | Cada 2 000 ms |
| Salida por consola | Cada 2 000 ms |

### Salida por puerto serie (115200 baud)

```
===========================================
  Chaleco Inteligente – ESP32 iniciado
  HC-SR04 | DHT11 | Semaforo | Buzzer THDZ
===========================================
-------------------------------------------
  Distancia  : 24.3 cm
  Temperatura: 23.5 °C
  Humedad    : 55.0 %
  Semaforo   : AMARILLO (10-30 cm) - 2 pitidos/s
-------------------------------------------
  Distancia  : 45.0 cm
  Temperatura: 23.5 °C
  Humedad    : 55.0 %
  Semaforo   : VERDE   (> 30 cm)  - sin pitido
-------------------------------------------
```

### Buzzer no bloqueante

El buzzer se gestiona sin usar `delay()`, mediante temporizadores con `millis()`:

- Si la distancia es **> 30 cm**, el pin se pone a `LOW` inmediatamente y se sale de la función.
- En caso contrario, se activa el pin (`HIGH`) durante **80 ms** y luego se desactiva (`LOW`).
- El período entre pitidos depende de la distancia:
  - 10–30 cm → período 500 ms (2 pitidos/s)
  - < 10 cm  → período 200 ms (5 pitidos/s)

---

## Dependencias (PlatformIO)

```ini
lib_deps =
    adafruit/DHT sensor library @ ^1.4.6
    adafruit/Adafruit Unified Sensor @ ^1.1.14
```

---

## Compilar y cargar

```bash
# Compilar
pio run

# Compilar y subir
pio run --target upload

# Monitor serie
pio device monitor
```

---

## Estructura del proyecto

```
chaleco/
├── platformio.ini      ← Configuración de la placa y librerías
├── README.md           ← Este archivo
├── src/
│   └── main.cpp        ← Código principal
├── include/
├── lib/
└── test/
```
