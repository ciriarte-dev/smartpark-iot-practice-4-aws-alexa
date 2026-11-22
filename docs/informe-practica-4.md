# Práctica 4 — Procesamiento, Almacenamiento y Lógica de Negocio de Objetos Inteligentes con AWS y Alexa

**SmartPark — Integración de un objeto inteligente con AWS (IoT Core, IoT Rules, Lambda y DynamoDB) y Alexa, con procesamiento, almacenamiento y lógica de control en el Edge y/o en la Nube**

**Carrera:** Ingeniería de Sistemas

**Docente:** Eduardo Enrique Marin Garcia

**Asignatura:** Internet de las Cosas [SIS-234]

**Semestre:** 2-2026

**Grupo:** 9

**Integrantes:**

- Carlos Eduardo Iriarte Rodriguez
- Isabella Catherine Montes Iñiguez
- Henry Alejandro Rivera Mendez

---

## Índice

1. [Caso de Uso y Requerimientos](#1-caso-de-uso-y-requerimientos)
2. [Análisis y Diseño](#2-análisis-y-diseño)
3. [Desarrollo e Implementación](#3-desarrollo-e-implementación)
4. [Pruebas y Validaciones](#4-pruebas-y-validaciones)
5. [Resultados](#5-resultados)
6. [Conclusiones](#6-conclusiones)
7. [Recomendaciones](#7-recomendaciones)
8. [Anexos](#8-anexos)

**Estado de la evidencia.** El diseño y la implementación descritos en las secciones 1 a 3 están en el repositorio y se enlazan en cada punto. Las pruebas automáticas se ejecutaron el 2026-11-21 (sección 4.5). El despliegue en AWS, las pruebas con el hardware y las pruebas en el simulador de Alexa se ejecutan en la sesión de banco descrita en 4.1; sus celdas indican *Pendiente de ejecución* hasta que la evidencia esté en [`docs/evidencias/`](evidencias/).

---

## 1. Caso de Uso y Requerimientos

### 1.1 Caso de uso y ampliación respecto a la Práctica 3

**Problema.** Un parqueo pequeño de acceso controlado, con **cuatro plazas** y una barrera de entrada, necesita tres cosas que un guardia resuelve hoy a ojo: saber cuántas plazas quedan libres, no dejar entrar vehículos cuando el parqueo está lleno y registrar cómo se usa el parqueo para decidir horarios, tarifas o ampliaciones.

**Base heredada (Práctica 3).** El objeto inteligente `smartpark-grupo9-01` (ESP32) medía la distancia con un HC-SR04 frente a la barrera y movía un servo SG90 solo por voz, a través del Device Shadow, con una skill multiusuario y vinculación por PIN temporal. Código e informe de la P3: <https://github.com/HenryRiveraM/smartpark-practica-3>.

**Ampliación en la Práctica 4.**

| Ámbito | Práctica 3 | Práctica 4 (este repositorio) |
|---|---|---|
| Sensores | HC-SR04 | HC-SR04 + **4 sensores IR, uno por plaza** ([`SLOT_PINS`](../include/config.example.h)) |
| Actuadores e indicadores | SG90 | SG90 + **8 LEDs** (rojo/verde por plaza) + **LCD 20×4 I²C** ([`ParkingDisplay`](../src/ParkingDisplay.cpp)) |
| Lógica de control | Ninguna: la barrera solo se mueve por orden | **Modos `manual` / `automatico`**; en automático el Edge **bloquea la apertura sin plazas confirmadas libres** ([`Parking::State::openingBlocked()`](../include/Parking.h#L15)) |
| Parámetros remotos | — | `modo` y `parametros_auto.antirrebote_ir_ms`, persistidos en NVS ([`applyDesired()`](../src/AwsIoTManager.cpp#L241)) |
| Histórico | — | Tópico `smartpark/<thingName>/history/v1` → regla `SmartParkHistoryIngest` → Lambda `SmartParkHistoryProcessor` → tabla `SmartParkHistory` |
| Skill | Barrera, distancia, vehículo, vinculación | + plazas disponibles/ocupadas, estado por plaza, modo y antirrebote (RF14) |
| Reportes | — | 4 mockups para el administrador (2.12) |

**Actores.**

| Actor | Rol |
|---|---|
| Conductor | Llega a la barrera; ocupa y libera plazas (detectado por los IR). |
| Usuario de Alexa (encargado) | Consulta disponibilidad, abre/cierra la barrera y cambia modo y antirrebote por voz. |
| Administrador | Genera el PIN de vinculación con `SmartParkPairingAdmin` y consulta los reportes de 2.12. |
| Objeto inteligente | Thing `smartpark-grupo9-01`: mide, decide en el Edge, actúa y publica estado e histórico. |

**Escenario.** Con el parqueo en `automatico`, el cuarto vehículo ocupa la plaza 4: los IR confirman la ocupación tras el antirrebote, el LED de la plaza pasa a rojo, la LCD muestra `PARQUEO LLENO`/`APERTURA BLOQUEADA` y el Shadow reporta `disponibles: 0` y `bloqueo_apertura: true`. Si alguien pide *"abre la barrera"*, el Edge rechaza la apertura y lo informa en `reported.ultima_orden.resultado = "bloqueada"`. Al liberarse una plaza, la apertura vuelve a permitirse sin intervención. Cada transición queda en `SmartParkHistory` para los reportes.

**Límites.** El SG90 no tiene realimentación: la posición de la barrera es una **estimación temporal** (`SERVO_TRAVEL_MS = 700` ms, `reported.barrera_posicion_estimada`). La barrera no es un dispositivo de seguridad ni detecta obstáculos. Todo el montaje opera a 5 V y 3,3 V (≤ 12 V DC).

### 1.2 Justificación de los sensores y actuadores

| Componente | Cantidad | Variable / acción | Por qué esa cantidad y tipo | Alternativas descartadas |
|---|---|---|---|---|
| Sensor IR de reflexión (salida digital) | 4 (uno por plaza) | Plaza ocupada / libre | La disponibilidad es la información central del caso de uso y se decide **por plaza**; un sensor por plaza permite contar y saber **cuál** está libre (consulta `ConsultarEstacionamientoIntent`). Es digital, barato, sin contacto y de respuesta rápida; el antirrebote por software (`IR_DEBOUNCE_*`) filtra el ruido de entrada. | Contador único en la entrada (pierde la plaza concreta y acumula error si falla una lectura); ultrasónico por plaza (más caro, 2 GPIO por plaza, más lento); sensor magnético en el piso (instalación invasiva). |
| HC-SR04 (ultrasónico) | 1 (frente a la barrera) | Distancia al vehículo y presencia con histéresis (15 cm / +3 cm) | Se conserva de la P3 porque responde una pregunta distinta de los IR: **si hay un vehículo esperando en la barrera** y a qué distancia (`ConsultarVehiculoIntent`, `ConsultarDistanciaIntent`). Los IR solo ven las plazas. No se usa para la decisión automática. | Un quinto IR en la entrada (sí/no sin distancia ni umbral configurable). |
| Servo SG90 | 1 | Barrera 0° (cerrada) ↔ 90° (abierta) | Dos posiciones discretas con una señal PWM, a 5 V. Es el actuador que no es LED exigido por el enunciado. | Motor DC con puente H (necesita finales de carrera); solenoide (carrera corta, consumo sostenido). |
| LED rojo + LED verde | 8 (2 por plaza) | Indicación local de ocupación | El conductor ve desde la entrada qué plaza está libre sin consultar a Alexa. Dos colores permiten un tercer estado visible: ambos apagados = `desconocido`. | LED RGB (más pines por plaza o un driver). |
| LCD 20×4 con PCF8574 | 1 | Ocupación, modo, barrera y aviso de lleno/bloqueo | Muestra el resumen del parqueo y la razón de un bloqueo en el sitio, también sin conexión. El backpack I²C usa solo 2 GPIO. | LCD 16×2 (no cabe el resumen en 4 líneas). |

Los pines y su adaptación eléctrica están en 2.3.

### 1.3 Requerimientos funcionales

Las pruebas `PF-xx` están en 4.2 y las automáticas `PA-xx` en 4.5.

| ID | Requerimiento (verificable) | Implementación |
|---|---|---|
| RF1 | Capturar la distancia cada 500 ms (`SENSOR_INTERVAL_MS`) y el estado de cada plaza muestreando los IR cada 10 ms (`IR_SAMPLE_MS`) con antirrebote configurable de 50 a 1000 ms. | [`UltrasonicSensor::measure()`](../src/UltrasonicSensor.cpp#L11), [`Parking::update()`](../src/Parking.cpp#L27) |
| RF2 | Reportar en `state.reported` la distancia, la presencia, la barrera, las plazas, los conteos, el modo y los parámetros: por evento y con respaldo cada 30 s (`AWS_REPORT_INTERVAL_MS`). | [`AwsIoTManager::report()`](../src/AwsIoTManager.cpp#L409), [`step()`](../src/AwsIoTManager.cpp#L464) |
| RF3 | Recibir el `delta`, aplicarlo (barrera, modo, antirrebote) y confirmar en `reported`. | [`receive()`](../src/AwsIoTManager.cpp#L329), [`applyDesired()`](../src/AwsIoTManager.cpp#L241), [`applyPending()`](../src/AwsIoTManager.cpp#L29) |
| RF4 | Responder por voz las consultas de sensores leyendo `reported` del Thing vinculado, solo con datos de ≤ 180 s. | [`sensor_handlers.py`](../lambda/handlers/sensor_handlers.py), [`ShadowService.get_reported_fields`](../lambda/services/shadow_service.py#L38) |
| RF5 | Ejecutar abrir/cerrar por voz escribiendo `desired`. | [`barrier_handlers.py`](../lambda/handlers/barrier_handlers.py), [`set_barrier`](../lambda/services/shadow_service.py#L76) |
| RF6 | Responder el estado reportado de la barrera. | [`EstadoBarreraHandler`](../lambda/handlers/barrier_handlers.py#L27) |
| RF7 | Vincular usuario ↔ Thing con PIN temporal (300 s, 3 intentos) y persistencia entre sesiones. | [`binding_handlers.py`](../lambda/handlers/binding_handlers.py), [`dynamodb_service.py`](../lambda/services/dynamodb_service.py) |
| RF8 | Resolver el Thing por `context.System.user.userId` y guiar si no hay vinculación. | [`bound_device()`](../lambda/handlers/common.py#L13) |
| RF9 | Validar que el Thing exista y permitir cambiar y eliminar la vinculación por voz. | [`CambiarVinculacionHandler`, `DesvincularParqueoHandler`](../lambda/handlers/binding_handlers.py#L30) |
| RF10 | Disparar la regla `SmartParkHistoryIngest` con cada mensaje de `smartpark/+/history/v1` e invocar `SmartParkHistoryProcessor`. | [`iot-rule.example.json`](../aws/history/iot-rule.example.json), [`HistoryPublisher::publish()`](../src/HistoryPublisher.cpp#L135) |
| RF11 | Validar tipos, rangos y coherencia; descartar inválidos sin almacenarlos; enriquecer con `thingName`, `schemaVersion` y `receivedAt`; guardar lecturas (`sample`, `occupancy_changed`) en `SmartParkHistory`. | [`validate_event()`](../lambda/history_lambda_function.py#L87), [`lambda_handler`](../lambda/history_lambda_function.py#L115) |
| RF12 | Registrar cada cambio de estado de la barrera (`barrier_changed`) con su origen (`voz`, `edge`, `nube`, `legado`). | [`HistoryPublisher::observeBarrier()`](../src/HistoryPublisher.cpp#L81), [`validate_barrier()`](../lambda/history_lambda_function.py#L74) |
| RF13 | Lógica automática en el Edge: (E1) indicación de plaza por LED/LCD; (E2) en `automatico`, bloquear la apertura si no hay plaza confirmada libre; (E3) el cierre siempre se acepta; (E4) una orden remota vencida (> 30 s) no mueve la barrera. | [`Parking`](../src/Parking.cpp), [`applyPending()`](../src/AwsIoTManager.cpp#L29), [`main.cpp`](../src/main.cpp#L62) |
| RF14 | Reportar `modo` y `parametros_auto.antirrebote_ir_ms` en `reported`, y consultarlos y cambiarlos por voz a través de `desired`. | [`parking_handlers.py`](../lambda/handlers/parking_handlers.py), [`set_mode`, `set_debounce`](../lambda/services/shadow_service.py#L81) |
| RF15 | Diseñar mockups de reportes obtenibles con consultas a `SmartParkHistory`. | [2.12](#212-diseño-de-reportes-mockups), [`docs/mockups/`](mockups/) |

### 1.4 Requerimientos no funcionales

Valores declarados, método y prueba. Los RNF1–RNF6 heredan los de la P3 y se extienden a los IR, LEDs y LCD.

| ID | Atributo | Valor declarado | Método de medición (origen de tiempo) | Prueba |
|---|---|---|---|---|
| RNF1 | Estabilidad | ≥ 15 min conectado con uso normal: 0 reinicios, 0 bloqueos, 0 reconexiones no provocadas. | Monitor serie con marca del PC; banner `=== SmartPark V2` y `reconexiones=` de `STATUS`. | PNF-01 |
| RNF2 | Exactitud HC-SR04 | Error absoluto máximo ≤ 1 cm entre 5 y 50 cm. | Regla + 5 lecturas `DISTANCE` por distancia. | PNF-02 |
| RNF3 | Exactitud IR | ≥ 95 % de transiciones correctas (≥ 76 de 80: 10 entradas y 10 salidas por plaza). | Conteo manual frente a `SLOT1`, LEDs y `reported.slots`. | PNF-03 |
| RNF4 | Tiempo de respuesta por voz | ≤ 3,0 s desde el fin de la frase hasta el fin del movimiento estimado del servo. | Video ≥ 30 fps, conteo de cuadros. | PNF-04 |
| RNF5 | Frecuencia de reporte | Shadow: por evento y respaldo cada 30 s; separación mínima 10 s salvo control y presencia; ≤ 1000 solicitudes/día (200 reservadas a control y recuperación). Histórico: muestra cada 300 s y ≤ 1 publicación cada 10 s. | Cliente MQTT de prueba y contadores de `STATUS`; ítems `sample` en DynamoDB. | PNF-08, PA-02 |
| RNF6 | Reconexión y sincronización | Con un corte de 30 s, MQTT reconecta en ≤ 60 s y el `desired.modo` escrito durante el corte se refleja en `reported` en ≤ 70 s, ambos desde que vuelve la red. | Monitor serie + `update/accepted` en el cliente MQTT. | PNF-09 |
| RNF7 | Integridad del almacenamiento | ≥ 99 % de los eventos publicados (contador `Historico: MQTT local`) quedan en `SmartParkHistory` en una ventana de 10 min; 100 % de los inválidos descartados. | Contador serie frente a ítems con `receivedAt` en la ventana; CloudWatch. | PNF-07 |
| RNF8 | Latencia de almacenamiento | ≤ 5 s desde la llegada a AWS IoT (`receivedAt`) hasta el fin del `PutItem` (registro `REPORT` en CloudWatch). Complementario: `receivedAt − occurredAt` ≤ 15 s (cola del ESP32 de hasta 10 s + red). | Atributos del ítem y hora de CloudWatch, ≥ 20 eventos. | PNF-06 |
| RNF9 | Respuesta de la lógica automática | Edge: ≤ 0,3 s desde que el vehículo cubre el IR hasta el cambio de LED (antirrebote de 120 ms + muestreo de 10 ms + ciclo de `loop()`). Nube: no aplica (2.6). | Video ≥ 30 fps. | PNF-05 |
| RNF10 | Operación sin conexión | E1–E4 siguen operando; los eventos de histórico se encolan en RAM (8 de barrera, 16 de plazas, 1 muestra) y se envían al reconectar; se pierden si la cola se llena o el ESP32 se reinicia. | Corte de 2 min; conteo de transiciones frente a ítems. | PNF-10 |
| RNF11 | Costo | Criterio principal: consumo mensual de un Thing dentro de la capa gratuita. Cota sin capa gratuita: < 0,30 USD/mes, que cubre el peor caso calculado (≈ 0,23 USD/mes con el Shadow facturado por bloques de 1 KB del documento completo). | Cálculo con las frecuencias del código. | 5.6 |
| RNF12 | Seguridad | Sin certificados, claves ni *access keys* en el repositorio; TLS mutuo X.509; políticas por recurso (3.2). | Revisión de `.gitignore` y plantillas; pruebas de aislamiento. | PA-03 |

Justificación de los valores: 3 s es el tiempo que un usuario espera tras la respuesta de Alexa antes de repetir la orden; 0,3 s es imperceptible para el conductor que mira el LED; 5 s de latencia de almacenamiento es la referencia del enunciado y basta para reportes que se consultan después; el límite de 60/70 s de RNF6 sale del *backoff* del código (2.10).

### 1.5 Trazabilidad de los requerimientos

| Req. | Componente / clase / archivo | Diagrama | Prueba |
|---|---|---|---|
| RF1 | `UltrasonicSensor`, `Parking` | 2.1, 2.4.1, 2.6 | PF-01, PF-11, PNF-02, PNF-03, PA-02 |
| RF2 | `AwsIoTManager::report()` | 2.5.1, 2.7 | PF-01, PNF-08, PA-02 |
| RF3 | `AwsIoTManager::applyDesired()` / `applyPending()` | 2.5.1 | PF-02, PF-15, PA-02 |
| RF4–RF6 | `sensor_handlers`, `barrier_handlers`, `ShadowService` | 2.4.2, 2.5.1 | PF-02, PF-03, PA-03 |
| RF7–RF9 | `binding_handlers`, `DynamoDBService`, `SmartParkPairingAdmin` | 2.2, 2.8 | PF-04, PF-05, PA-03 |
| RF10 | `HistoryPublisher`, regla `SmartParkHistoryIngest` | 2.2, 2.5.2, 2.9 | PF-06, PA-03 (`test_templates_keep_rule_identity_and_table_keys_aligned`) |
| RF11 | `history_lambda_function.validate_event()` | 2.5.2, 2.8 | PF-07, PF-08, PF-09, PNF-07, PA-03 (`test_snapshot_item_and_reduced_capacity`, `test_invalid_never_initializes_sdk`, `test_put_conditional_duplicate_and_retry`) |
| RF12 | `HistoryPublisher::observeBarrier()`, `validate_barrier()` | 2.5.2 | PF-10, PA-03 (`test_barrier_origins_and_actual_transitions`) |
| RF13 | `Parking::State::openingBlocked()`, `applyPending()`, `main.cpp` | 2.5.3, 2.6 | PF-11 a PF-14, PNF-05, PNF-10, PA-02 |
| RF14 | `parking_handlers`, `ShadowService.set_mode/set_debounce`, `applyDesired()` | 2.5.1, 2.7, 2.11 | PF-15, PF-16, PA-03 (`test_v2_*`) |
| RF15 | `SmartParkHistory` y patrones de acceso | 2.8, 2.12 | PF-17 |
| RNF1–RNF12 | ver 1.4 | 2.6, 2.10 | PNF-01 a PNF-10, 5.6, PA-01 a PA-03 |

---

## 2. Análisis y Diseño

### 2.1 Diagrama de bloques del objeto inteligente

El ESP32 separa el trabajo en dos contextos: `loop()` lee sensores, decide y acciona (Edge); la tarea FreeRTOS `smartpark-aws` es la única dueña de MQTT y NVS. Se comunican por tres colas de un elemento (instantánea, comando y estado), de modo que una red lenta nunca detiene la lógica local. El servo tiene su propia fuente de 5 V con masa común. Copia: [`diagrams/01-bloques.mmd`](diagrams/01-bloques.mmd).

```mermaid
flowchart LR
    subgraph PWR["Alimentación"]
        USB["USB 5 V<br/>(ESP32, LCD, IR, HC-SR04)"]
        EXT["Fuente regulada 5 V<br/>(solo SG90)"]
        GND(("GND común"))
    end
    subgraph SENS["Sensores"]
        US["HC-SR04<br/>TRIG GPIO 32 · ECHO GPIO 35<br/>(divisor 5 V → 3,3 V)"]
        IR["4 sensores IR, uno por plaza<br/>GPIO 34 · 36 · 39 · 33"]
    end
    subgraph MCU["ESP32 DOIT DevKit V1 · Thing smartpark-grupo9-01"]
        LOOP["loop(): Parking · UltrasonicSensor<br/>BarrierServo · ParkingDisplay · SerialCommands"]
        TASK["Tarea FreeRTOS smartpark-aws<br/>AwsIoTManager + HistoryPublisher"]
        NVS[("NVS<br/>modo, antirrebote,<br/>cuota diaria, órdenes")]
        LOOP <-- "colas de 1 elemento<br/>(snapshot, comando, estado)" --> TASK
        TASK --- NVS
    end
    subgraph ACT["Actuadores e indicadores"]
        SERVO["Servo SG90 (barrera)<br/>PWM 50 Hz · GPIO 18"]
        LEDS["8 LEDs: rojo/verde por plaza<br/>17/13 · 14/19 · 23/25 · 26/27"]
        LCD["LCD 20x4 + PCF8574 (0x27)<br/>SDA GPIO 21 · SCL GPIO 22"]
    end
    CLOUD["AWS IoT Core<br/>MQTT sobre TLS 8883"]
    PC["Monitor serie 115200 baudios<br/>OPEN · CLOSE · STATUS · DISTANCE · SLOT1 · HELP"]

    US --> LOOP
    IR --> LOOP
    LOOP --> SERVO
    LOOP --> LEDS
    LOOP --> LCD
    TASK <-- "Wi-Fi" --> CLOUD
    PC <--> LOOP
    USB --> MCU
    EXT --> SERVO
    GND --- MCU
    GND --- SERVO
```

### 2.2 Diagrama de arquitectura

![Arquitectura de SmartPark P4](diagrams/02-arquitectura.png)

Fuente: [`diagrams/02-arquitectura.html`](diagrams/02-arquitectura.html). Hay dos flujos independientes que se encuentran solo en AWS IoT Core: el de **control** (Alexa → `SmartParkAlexaBackend` → Shadow → ESP32) y el de **histórico** (ESP32 → `smartpark/+/history/v1` → `SmartParkHistoryIngest` → `SmartParkHistoryProcessor` → `SmartParkHistory`). Las etiquetas de las flechas son las acciones IAM del rol de origen (3.2). Ni Alexa ni las Lambdas hablan con el ESP32: solo escriben `desired`.

### 2.3 Diagrama de circuito

![Circuito de SmartPark V2](diagrams/03-circuito.png)

Fuente editable: [`diagrams/03-circuito.excalidraw`](diagrams/03-circuito.excalidraw).

| Pin ESP32 | Componente | Alimentación | Observaciones |
|---|---|---|---|
| GPIO 32 | HC-SR04 TRIG | 5 V | Salida de 3,3 V; el HC-SR04 la acepta como nivel alto. |
| GPIO 35 | HC-SR04 ECHO | 5 V | Solo entrada. ECHO sale a 5 V: adaptar a 3,3 V (divisor) antes del pin. |
| GPIO 34 / 36 / 39 / 33 | IR plazas 1 / 2 / 3 / 4 | 3,3 V | 34, 36 y 39 son solo entrada y sin pull-up interno: una salida de colector abierto requiere pull-up externo a 3,3 V. Activos en bajo (`IR_OCCUPIED_LEVEL = LOW`). |
| GPIO 18 | SG90 señal | 5 V de fuente externa | PWM 50 Hz, 500–2400 µs (0° = 500 µs, 90° = 1450 µs). Nunca alimentar desde 3V3 ni desde un GPIO. |
| GPIO 17 / 13 | LED rojo / verde plaza 1 | GPIO | Una resistencia por LED: R ≥ (3,3 V − Vf) / 2 mA. GPIO 17 debe estar libre de PSRAM en el módulo. |
| GPIO 14 / 19 | LED rojo / verde plaza 2 | GPIO | Ídem. |
| GPIO 23 / 25 | LED rojo / verde plaza 3 | GPIO | Ídem. |
| GPIO 26 / 27 | LED rojo / verde plaza 4 | GPIO | Ídem. |
| GPIO 21 / 22 | LCD SDA / SCL (PCF8574, 0x27) | 5 V (backpack) | I²C a 100 kHz. Con backpack a 5 V, usar traductor de nivel bidireccional. |
| VIN / GND | USB 5 V / masa común | — | Masa común con la fuente del servo; positivos separados. |

Fuente de los pines: [`config.example.h`](../include/config.example.h). Notas de cableado y presupuesto de corriente: [`firmware-v2.md`](firmware-v2.md#cableado-previsto).

### 2.4 Diagramas de clases

#### 2.4.1 Objeto inteligente (firmware ESP32)

Cada clase encapsula un periférico o una responsabilidad. `AwsIoTManager` contiene a `ShadowBudget` (cuota diaria en NVS) y a `HistoryPublisher` (colas del histórico); actúa sobre `Parking` y `BarrierServo` solo desde `applyPending()`, en el contexto de `loop()`. Copia: [`diagrams/04-clases-firmware.mmd`](diagrams/04-clases-firmware.mmd).

```mermaid
classDiagram
    direction LR
    class main {
        <<sketch Arduino>>
        +setup()
        +loop()
        -execute(Command)
    }
    class UltrasonicSensor {
        -Measurement last_
        -Presence presence_
        +begin()
        +measure() Measurement
        +lastMeasurement() Measurement
        +presence() Presence
    }
    class BarrierServo {
        -Servo servo_
        -State commanded_
        -bool moving_
        -uint32_t commandTime_
        +open() bool
        +close() bool
        +update()
        +positionEstimated() bool
    }
    class Parking {
        +capacity = PARKING_CAPACITY
        -State state_
        -Slot candidate_[capacity]
        -uint32_t candidateSince_[capacity]
        +begin()
        +update(now)
        +configure(Mode, debounceMs)
        +state() State
    }
    class ParkingState {
        <<struct Parking::State>>
        +Slot slots[capacity]
        +Mode mode
        +uint16_t debounceMs
        +uint8_t occupied
        +uint8_t available
        +full() bool
        +openingBlocked() bool
    }
    class ParkingDisplay {
        -Init init_
        -char displayed_[4][21]
        +begin()
        +probe() uint8_t
        +update(now, State, BarrierServo)
    }
    class SerialCommands {
        -char buffer_[COMMAND_CAPACITY]
        +feed(char) Command
    }
    class AwsIoTManager {
        -WiFiClientSecure tls_
        -PubSubClient mqtt_
        -Preferences prefs_
        -QueueHandle_t snapshots_, commands_, statuses_
        -OrderLedger ledger_
        +begin(Parking) bool
        +applyPending(BarrierServo, Parking, serialPending)
        +tick(BarrierServo, UltrasonicSensor, Parking)
        +noteSerialCommand(open, blocked, available, moved)
        +observeBarrier(BarrierServo)
        +printStatus()
        -run()
        -step()
        -connectStep(now)
        -receive(topic, payload, length)
        -applyDesired(desired, metadata, get)
        -report(Kind)
    }
    class ShadowBudget {
        +ShadowBudgetRecord record
        +total() uint32_t
        +advance(day) bool
        +allows(Kind, limit, reserved) bool
    }
    class HistoryPublisher {
        -QueueHandle_t queues_[3]
        -uint32_t bootId_[4]
        -uint64_t sequence_
        +begin()
        +enableClock()
        +observeParking(State, UltrasonicSensor, now)
        +motionStarted(Origin, requestId)
        +observeBarrier(BarrierServo)
        +publish(PubSubClient, now)
        +stats() Stats
    }
    main *-- UltrasonicSensor
    main *-- BarrierServo
    main *-- Parking
    main *-- ParkingDisplay
    main *-- SerialCommands
    main *-- AwsIoTManager
    Parking *-- ParkingState
    AwsIoTManager *-- ShadowBudget
    AwsIoTManager *-- HistoryPublisher
    AwsIoTManager ..> Parking : configure()
    AwsIoTManager ..> BarrierServo : open() / close()
    HistoryPublisher ..> ParkingState : lee
    ParkingDisplay ..> ParkingState : lee
```

#### 2.4.2 Funciones Lambda

Tres paquetes de despliegue con responsabilidad única. El backend de la skill y la administración comparten solo `services.validation`; el procesador del histórico es un único archivo sin dependencias del SDK de Alexa, para que su ZIP sea mínimo. Copia: [`diagrams/05-clases-lambdas.mmd`](diagrams/05-clases-lambdas.mmd).

```mermaid
classDiagram
    direction TB
    namespace SmartParkAlexaBackend {
        class lambda_function {
            <<módulo>>
            +lambda_handler(event, context)
            -_safe_response(message)
        }
        class handlers {
            <<paquete>>
            barrier_handlers: Abrir, Cerrar, EstadoBarrera
            binding_handlers: Vincular, CambiarVinculacion, Desvincular
            sensor_handlers: ConsultarDistancia, ConsultarVehiculo
            parking_handlers: Disponibles, Ocupados, Estacionamiento, Modo, Antirrebote
            system_handlers: Launch, Help, Exit, SessionEnded, Fallback, SafeException
        }
        class ShadowService {
            +describe_thing(thing)
            +get_reported_fields(thing, paths) tuple
            +set_barrier(thing, state)
            +set_mode(thing, mode)
            +set_debounce(thing, ms)
            -_update_desired(thing, patch)
        }
        class DynamoDBService {
            +get_binding(user_id) Binding
            +authorize_code(user_id, code) PairingAuthorization
            +save_binding(user_id, authorization, previous)
            +delete_binding(user_id) bool
        }
        class runtime {
            <<módulo>>
            +get_backend() Backend
        }
    }
    namespace SmartParkPairingAdmin {
        class admin_lambda_function {
            <<módulo>>
            +lambda_handler(event, context)
        }
        class PairingAdminService {
            +generate(user_id, thing) dict
        }
    }
    namespace SmartParkHistoryProcessor {
        class history_lambda_function {
            <<módulo>>
            +lambda_handler(event, context)
            +validate_event(event) dict
            +validate_snapshot(data, transition) dict
            +validate_barrier(data) dict
            -dynamodb() client
        }
        class InvalidEvent {
            <<excepción>>
        }
    }
    class validation {
        <<módulo compartido>>
        +validate_user(value)
        +validate_thing(value)
        +pairing_key(value)
    }
    lambda_function --> handlers : SkillBuilder
    handlers --> runtime
    runtime --> ShadowService
    runtime --> DynamoDBService
    handlers ..> validation
    DynamoDBService ..> validation
    admin_lambda_function --> PairingAdminService
    PairingAdminService ..> validation
    history_lambda_function ..> InvalidEvent : lanza
```

### 2.5 Diagramas de secuencia

#### 2.5.1 Consulta del sensor y comando sobre el actuador

**Consulta** (*"cuántos espacios hay disponibles"*). La Lambda no responde con un dato viejo: comprueba el `timestamp` de cada atributo usado en `metadata.reported` (≤ 180 s, `MAX_REPORTED_AGE_SECONDS`). Copia: [`06-secuencia-consulta.mmd`](diagrams/06-secuencia-consulta.mmd).

```mermaid
sequenceDiagram
    autonumber
    actor U as Usuario
    participant AX as Skill Alexa
    participant LA as SmartParkAlexaBackend
    participant TB as DynamoDB SmartParkBindings
    participant SH as Device Shadow
    U->>AX: "cuántos espacios hay disponibles"
    AX->>LA: IntentRequest ConsultarDisponiblesIntent + userId
    LA->>LA: valida ALEXA_SKILL_ID, edad de la solicitud (-30 a 150 s) y userId
    LA->>TB: GetItem(userId) ConsistentRead
    TB-->>LA: thingName = smartpark-grupo9-01
    LA->>SH: GetThingShadow(thingName)
    SH-->>LA: state.reported + metadata.reported
    alt timestamp de disponibles y capacidad con antigüedad ≤ 180 s
        LA-->>AX: "El último reporte indica 2 espacios disponibles de 4."
    else dato desactualizado o ausente
        LA-->>AX: "El último dato del dispositivo está desactualizado..."
    end
    AX-->>U: respuesta hablada
```

**Comando sobre el actuador** (*"abre la barrera"*). La Lambda solo escribe `desired.barrera`; la decisión de mover o bloquear la toma el Edge y queda en `reported.ultima_orden`. Copia: [`07-secuencia-comando.mmd`](diagrams/07-secuencia-comando.mmd).

```mermaid
sequenceDiagram
    autonumber
    actor U as Usuario
    participant LA as SmartParkAlexaBackend
    participant SH as Device Shadow
    participant NET as ESP32 tarea smartpark-aws
    participant LP as ESP32 loop()
    participant SV as BarrierServo (SG90)
    U->>LA: "abre la barrera" (AbrirBarreraIntent)
    LA->>SH: UpdateThingShadow {"state":{"desired":{"barrera":"abierta"}}}
    LA-->>U: "Solicité abrir la barrera..."
    SH->>NET: $aws/things/smartpark-grupo9-01/shadow/update/delta
    NET->>NET: receive() → applyDesired(): versión, timestamp AWS < 30 s, registro de la orden en NVS
    NET->>LP: cola commands_ (Command)
    LP->>LP: applyPending(): ¿orden vencida? ¿openingBlocked()?
    alt modo automatico y disponibles == 0
        LP->>LP: ultima_orden.resultado = bloqueada (sin PWM)
    else permitida
        LP->>SV: open() → writeMicroseconds(1450 µs)
        Note over SV: SERVO_TRAVEL_MS = 700 ms (posición estimada)
        LP->>LP: ultima_orden.resultado = aceptada
    end
    LP->>NET: cola snapshots_ (tick)
    NET->>SH: shadow/update reported {barrera, ultima_orden, ...} (clase Control)
    SH-->>NET: update/accepted (mismo clientToken)
```

**Cambio de parámetro por voz (RF14).** La validación ocurre dos veces: en la Lambda (50–1000) y en el firmware, que además persiste el valor en NVS antes de aplicarlo. Copia: [`07b-secuencia-modo.mmd`](diagrams/07b-secuencia-modo.mmd).

```mermaid
sequenceDiagram
    autonumber
    actor U as Usuario
    participant LA as SmartParkAlexaBackend
    participant SH as Device Shadow
    participant NET as ESP32 tarea smartpark-aws
    participant LP as ESP32 loop()
    U->>LA: "ajusta el antirrebote a 200 milisegundos"
    LA->>LA: number_slot(milisegundos, 50, 1000)
    alt valor fuera de 50–1000
        LA-->>U: "Dime un valor entero entre 50 y 1000 milisegundos." (sin escribir el Shadow)
    else válido
        LA->>SH: UpdateThingShadow desired.parametros_auto.antirrebote_ir_ms = 200
        LA-->>U: "Solicité 200 milisegundos de antirrebote. El dispositivo aún debe confirmarlo."
        SH->>NET: update/delta
        NET->>NET: applyDesired(): valida rango y guarda {modo, antirrebote} en NVS "parking"
        NET->>LP: cola commands_ (configure = true)
        LP->>LP: Parking::configure(): nueva ventana de estabilidad
        NET->>SH: reported.parametros_auto.antirrebote_ir_ms = 200 (clase Control)
    end
    U->>LA: "consulta el antirrebote"
    LA->>SH: GetThingShadow
    LA-->>U: "El antirrebote reportado es de 200 milisegundos."
```

#### 2.5.2 Almacenamiento de un dato (objeto → regla de IoT → Lambda → DynamoDB)

Incluye el descarte de inválidos y la deduplicación por escritura condicional. Copia: [`08-secuencia-almacenamiento.mmd`](diagrams/08-secuencia-almacenamiento.mmd).

```mermaid
sequenceDiagram
    autonumber
    participant LP as ESP32 loop()
    participant HP as HistoryPublisher (colas RAM)
    participant NET as Tarea smartpark-aws
    participant BRK as AWS IoT Core (broker)
    participant RULE as Regla SmartParkHistoryIngest
    participant LH as SmartParkHistoryProcessor
    participant TH as DynamoDB SmartParkHistory
    participant CW as CloudWatch Logs
    LP->>HP: observeParking() / observeBarrier()
    HP->>HP: enqueue(): exige hora UTC válida, occurredAt = gettimeofday (ms)
    Note over HP: colas FIFO: barrera 8 · plazas 16 · muestra 1<br/>cola llena → se descarta el evento nuevo
    NET->>HP: publish() como máximo cada 10 s, prioridad barrera > plazas > muestra
    HP->>BRK: PUBLISH QoS 0 smartpark/smartpark-grupo9-01/history/v1
    BRK->>RULE: mensaje del tópico
    RULE->>RULE: agrega thingName = topic(2), schemaVersion = topic(4), receivedAt = timestamp()
    RULE->>LH: invocación asíncrona
    LH->>LH: validate_event(): tipos, rangos, conteos, origen, occurredAt ≤ receivedAt + 300 s
    alt evento inválido
        LH->>CW: WARNING history_invalid field=...
        LH-->>RULE: {"status":"invalid"} (no se almacena, sin reintento)
    else válido
        LH->>TH: PutItem eventKey = occurredAt(13 dígitos)#eventId<br/>condición attribute_not_exists
        alt primera escritura
            TH-->>LH: OK → {"status":"stored"}
        else mismo eventId y occurredAt
            TH-->>LH: ConditionalCheckFailed → {"status":"duplicate"}
        end
    end
```

#### 2.5.3 Ejecución de la regla automática en el Edge

**E1 — indicación de plaza.** Copia: [`09-secuencia-edge-plaza.mmd`](diagrams/09-secuencia-edge-plaza.mmd).

```mermaid
sequenceDiagram
    autonumber
    actor V as Vehículo (maqueta)
    participant IR as Sensor IR plaza n
    participant PK as Parking::update()
    participant LED as LEDs plaza n
    participant LCD as ParkingDisplay
    participant HP as HistoryPublisher
    participant NET as Tarea smartpark-aws
    V->>IR: cubre la plaza
    IR-->>PK: digitalRead = IR_OCCUPIED_LEVEL (LOW), muestreo cada 10 ms
    PK->>PK: candidato estable durante debounceMs (120 ms por defecto)
    PK->>LED: rojo ON, verde OFF
    PK->>PK: occupied++, available--
    LCD->>LCD: "Ocup:x Libres:y/4" y, si available == 0, "PARQUEO LLENO" o "APERTURA BLOQUEADA"
    PK-->>HP: observeParking(): occupancy_changed (slot, estado_anterior)
    PK-->>NET: snapshot → reported.slots / ocupados / disponibles / lleno / bloqueo_apertura (clase Sensor)
```

**E2, E3 y E4 — bloqueo de apertura, cierre siempre permitido y vencimiento de órdenes.** La misma regla se aplica a la orden por voz y a `OPEN` por Serial ([`main.cpp`](../src/main.cpp#L62)). Copia: [`10-secuencia-edge-bloqueo.mmd`](diagrams/10-secuencia-edge-bloqueo.mmd).

```mermaid
sequenceDiagram
    autonumber
    participant SRC as Origen de la orden<br/>(voz vía Shadow o Serial OPEN)
    participant LP as loop() / applyPending()
    participant PK as Parking::State
    participant SV as BarrierServo
    participant NET as Tarea smartpark-aws
    SRC->>LP: orden de apertura
    LP->>LP: orden remota: ¿now < vence (≤ 30 s)? si no → vencida
    LP->>PK: openingBlocked() = modo automatico y available == 0
    alt bloqueada
        LP-->>SRC: Serial: "Apertura bloqueada: sin plazas confirmadas"
        LP->>NET: ultima_orden {objetivo: abierta, resultado: bloqueada}
        Note over SV: sin pulso PWM, sin evento barrier_changed
    else permitida (modo manual o available ≥ 1)
        LP->>SV: open()
        LP->>NET: ultima_orden {resultado: aceptada}
        Note over SV: tras 700 ms: barrier_changed al histórico
    end
    NET->>NET: report(Control) con bloqueo_apertura y ultima_orden
```

#### 2.5.4 Ejecución de las reglas automáticas en la Nube

No aplica: el repositorio no implementa reglas de control en la Nube. La única Lambda disparada por una regla de IoT, `SmartParkHistoryProcessor`, valida y almacena, y su rol no tiene permiso `iot:UpdateThingShadow` ([`processor-execution-policy.example.json`](../aws/history/processor-execution-policy.example.json)), por lo que no puede actuar sobre la barrera. La justificación está en 2.6 y las reglas de Nube recomendadas para una versión real en la sección 7.

### 2.6 Lógica de control (diagrama de estados y ubicación Edge / Nube)

**Modo de operación.** El valor por defecto es `manual`; el modo y el antirrebote se guardan en NVS y sobreviven a un reinicio. Copia: [`11-estados-control.mmd`](diagrams/11-estados-control.mmd).

```mermaid
stateDiagram-v2
    direction LR
    [*] --> Manual : primer arranque (valor por defecto)
    Manual --> Automatico : desired.modo = automatico
    Automatico --> Manual : desired.modo = manual
    state Automatico {
        [*] --> AperturaPermitida
        AperturaPermitida --> AperturaBloqueada : disponibles == 0
        AperturaBloqueada --> AperturaPermitida : una plaza confirmada libre
    }
    note right of Manual : abrir y cerrar aceptados
    note right of Automatico : cerrar siempre aceptado
```

**Barrera (posición estimada).** Arranca sin estimar y sin PWM: ni el arranque, ni un cambio de modo, ni liberar una plaza mueven el servo. Copia: [`11b-estados-barrera.mmd`](diagrams/11b-estados-barrera.mmd).

```mermaid
stateDiagram-v2
    direction LR
    [*] --> SinEstimar : arranque, sin PWM
    SinEstimar --> Moviendo : primera orden explícita
    Moviendo --> AbiertaEstimada : 700 ms, objetivo abierta
    Moviendo --> CerradaEstimada : 700 ms, objetivo cerrada
    CerradaEstimada --> Moviendo : abrir aceptada
    AbiertaEstimada --> Moviendo : cerrar aceptada
    CerradaEstimada --> CerradaEstimada : abrir bloqueada o vencida
```

**Plaza.** Una lectura solo se acepta si se mantiene `debounceMs`; `desconocido` no cuenta como libre ni como ocupada. Copia: [`12-estados-plaza.mmd`](diagrams/12-estados-plaza.mmd).

```mermaid
stateDiagram-v2
    direction LR
    [*] --> Desconocido : arranque (LEDs apagados)
    Desconocido --> Libre : lectura HIGH estable ≥ debounceMs
    Desconocido --> Ocupado : lectura LOW estable ≥ debounceMs
    Libre --> Ocupado : LOW estable ≥ debounceMs
    Ocupado --> Libre : HIGH estable ≥ debounceMs
    Libre --> Desconocido : lectura digital inválida
    Ocupado --> Desconocido : lectura digital inválida
    note right of Libre : LED verde ON
    note right of Ocupado : LED rojo ON
```

**Reglas y ubicación.**

| Regla | Condición → acción | Ubicación | Justificación |
|---|---|---|---|
| E1 Indicación de plaza | IR estable ≥ `debounceMs` → LED rojo/verde, conteos, LCD | Edge ([`Parking::update()`](../src/Parking.cpp#L27)) | Depende solo de datos locales; latencia de decenas de ms; debe funcionar sin red. Llevarla a la Nube costaría un mensaje por transición y dejaría el LED apagado sin conexión. |
| E2 Bloqueo de apertura | `modo == automatico` y `disponibles == 0` → apertura rechazada (`bloqueada`) | Edge ([`openingBlocked()`](../include/Parking.h#L15), [`applyPending()`](../src/AwsIoTManager.cpp#L42), [`main.cpp`](../src/main.cpp#L62)) | La decisión usa el estado de las plazas que solo el ESP32 conoce al instante; en la Nube habría una carrera entre el último `reported` y la orden. Sigue operando sin red. |
| E3 Cierre permitido | En `automatico`, `cerrar` siempre se acepta | Edge | Cerrar nunca empeora la ocupación; bloquearlo dejaría la barrera abierta. |
| E4 Vencimiento de orden remota | `now ≥ vence` (≤ `AWS_ORDER_TTL_S` = 30 s) → `vencida`, sin movimiento | Edge ([`applyPending()`](../src/AwsIoTManager.cpp#L40)) | Evita que una orden dada durante un corte abra la barrera minutos después, cuando nadie la espera. |

**Por qué ninguna regla está en la Nube.** Las cuatro reglas dependen solo del estado local y deben cumplirse sin conexión (RNF10). Una regla en la Nube añadiría un viaje Shadow → regla → Lambda → `desired` → `delta` por decisión, más invocaciones y mensajes (2.10), y fallaría justo cuando el objeto pierde la red. Las decisiones que sí requieren histórico, varios Things o detectar silencio del objeto (por ejemplo, alertar si el Thing deja de reportar o limitar aperturas diarias) no forman parte de RF1–RF15 de este caso de uso y se proponen en 7.2. La Nube se encarga del **almacenamiento validado** y de los **reportes** (2.8, 2.12).

**Convivencia entre voz y lógica automática** (según el código):

| Situación | Resultado |
|---|---|
| `manual` + *"abre la barrera"* | Se acepta y mueve el servo (`aceptada`). |
| `automatico` con ≥ 1 plaza libre + *"abre la barrera"* | Se acepta. |
| `automatico` con 0 libres (o plazas `desconocido`) + *"abre la barrera"* | Se rechaza en el Edge: `ultima_orden.resultado = "bloqueada"`, LCD `APERTURA BLOQUEADA`. Alexa responde *"Solicité abrir la barrera…"* porque la Lambda no espera al dispositivo; *"consulta el estado de la barrera"* confirma que sigue cerrada. |
| Cualquier modo + *"cierra la barrera"* | Se acepta. |
| Orden Serial y orden remota en la misma vuelta de `loop()` | Prevalece la Serial (operador en sitio) y la remota se descarta ([`applyPending()`](../src/AwsIoTManager.cpp#L37)). |
| *"activa el modo manual"* | Desactiva el bloqueo: la voz no obliga a abrir, solo cambia el modo; la apertura requiere otra orden. |

### 2.7 Especificación del documento Shadow

Shadow clásico del Thing `smartpark-grupo9-01`. Lo escriben el ESP32 (`reported`, [`report()`](../src/AwsIoTManager.cpp#L409)) y `SmartParkAlexaBackend` (`desired`, [`_update_desired`](../lambda/services/shadow_service.py#L91)).

| Atributo | Tipo | Valores válidos | Sección | Escribe | Lee |
|---|---|---|---|---|---|
| `barrera` | string \| null | `abierta`, `cerrada`; `null` sin posición estimada | reported / desired | ESP32 / Lambda | Lambda (`EstadoBarreraIntent`) / ESP32 |
| `barrera_posicion_estimada` | bool | `true` tras un recorrido completo | reported | ESP32 | — |
| `distancia_cm` | number \| null | 2–400 (1 decimal); `null` si la lectura es inválida | reported | ESP32 | Lambda |
| `vehiculo_detectado` | bool \| null | `null` sin 3 muestras estables | reported | ESP32 | Lambda |
| `slots` | objeto `"1"`..`"4"` → string | `libre`, `ocupado`, `desconocido` | reported | ESP32 | Lambda (`ConsultarEstacionamientoIntent`) |
| `ocupados`, `disponibles` | int | 0–`capacidad` (`desconocido` no cuenta) | reported | ESP32 | Lambda |
| `capacidad` | int | 1–4 (`PARKING_CAPACITY`) | reported | ESP32 | Lambda |
| `lleno` | bool | `ocupados == capacidad` | reported | ESP32 | — |
| `bloqueo_apertura` | bool | `modo == automatico` y `disponibles == 0` | reported | ESP32 | — |
| `modo` | string | `manual`, `automatico` | reported / desired | ESP32 / Lambda | Lambda / ESP32 |
| `parametros_auto.antirrebote_ir_ms` | int | 50–1000 | reported / desired | ESP32 / Lambda | Lambda / ESP32 |
| `ultima_orden` | objeto | `id` (string \| null), `objetivo` (`abierta`/`cerrada`), `origen` (`voz`, `nube`, `edge`, `legado`), `resultado` (`aceptada`, `bloqueada`, `vencida`, `no_disponible`) | reported | ESP32 | consola / pruebas |
| `orden_barrera` | objeto | `id` 1–32 `[A-Za-z0-9_-]`, `objetivo`, `origen` (`voz`/`nube`), `solicitada_ms` (epoch ms), `vence_s` (epoch s, ≤ 30 s) | desired | cliente V2 (ninguno en el repositorio) | ESP32 |

`desired` escrito por la skill (formato V1 para la barrera, RF14 para modo y parámetro):

```json
{"state": {"desired": {"barrera": "abierta"}}}
{"state": {"desired": {"modo": "automatico"}}}
{"state": {"desired": {"parametros_auto": {"antirrebote_ir_ms": 200}}}}
```

`reported` emitido por el ESP32 (forma de [`report()`](../src/AwsIoTManager.cpp#L409); valores ilustrativos):

```json
{
  "state": {
    "reported": {
      "barrera_posicion_estimada": true, "barrera": "cerrada",
      "slots": {"1": "ocupado", "2": "ocupado", "3": "ocupado", "4": "ocupado"},
      "ocupados": 4, "disponibles": 0, "capacidad": 4,
      "modo": "automatico", "lleno": true, "bloqueo_apertura": true,
      "parametros_auto": {"antirrebote_ir_ms": 120},
      "ultima_orden": {"id": null, "objetivo": "abierta", "origen": "legado", "resultado": "bloqueada"},
      "distancia_cm": 12.4, "vehiculo_detectado": true
    }
  }
}
```

`delta` que recibe el ESP32 tras *"activa el modo manual"* con `reported.modo = "automatico"`:

```json
{"version": 128, "timestamp": 1795000000, "state": {"modo": "manual"},
 "metadata": {"modo": {"timestamp": 1795000000}}}
```

El firmware ordena GET y deltas por `version`, correlaciona los acuses por `clientToken` y, para el formato V1, identifica la orden por `metadata.barrera.timestamp` y la descarta si tiene más de 30 s ([`applyDesired()`](../src/AwsIoTManager.cpp#L301)). Formato V2 completo: [`firmware-v2.md`](firmware-v2.md#device-shadow-clásico).

### 2.8 Modelo de datos de DynamoDB

```mermaid
erDiagram
    SmartParkBindings {
        string userId PK "amzn1.ask.account... | PAIR#sha256 | PAIRUSER#sha256"
        string recordType "binding | pairing | pairing_window"
        string thingName
        string revision "uuid (binding)"
        string fechaVinculacion "ISO 8601 (binding)"
        string allowedUserId "pairing"
        number expiresAt "epoch s (pairing, pairing_window)"
        string generation "uuid"
        string pairKey "pairing_window"
        number remainingAttempts "pairing_window"
        string status "active | consumed"
    }
    SmartParkHistory {
        string thingName PK "clave de partición"
        string eventKey PK "clave de ordenamiento: occurredAt 13 dígitos # eventId"
        string schemaVersion "v1"
        string eventId
        number occurredAt "epoch ms, reloj del ESP32"
        number receivedAt "epoch ms, timestamp() de la regla"
        string eventType "sample | occupancy_changed | barrier_changed"
        string origin "edge | voz | nube | legado"
        map data "instantánea o transición"
    }
    SmartParkBindings ||--o{ SmartParkHistory : "thingName (relación lógica, sin clave foránea)"
```

Copia: [`13-modelo-datos.mmd`](diagrams/13-modelo-datos.mmd). Las dos tablas son independientes: la relación por `thingName` es lógica.

**`SmartParkBindings`** (heredada de la P3, bajo demanda, PK `userId` String). Una sola tabla guarda tres tipos de registro distinguidos por el prefijo de la clave: `amzn1.ask.account…` (vinculación), `PAIR#<sha256(PIN)>` (autorización) y `PAIRUSER#<sha256(userId)>` (ventana de intentos). Sin índices; los registros de PIN tienen `expiresAt` (epoch s), pero el repositorio no documenta TTL activado en la tabla: la caducidad se aplica por condición en el código ([`dynamodb_service.py`](../lambda/services/dynamodb_service.py)).

**`SmartParkHistory`** ([`table.example.json`](../aws/history/table.example.json), `PAY_PER_REQUEST`):

| Atributo | Tipo DynamoDB | Origen | Ejemplo |
|---|---|---|---|
| `thingName` (PK) | S | regla: `topic(2)` | `smartpark-grupo9-01` |
| `eventKey` (SK) | S | Lambda: `f"{occurredAt:013d}#{eventId}"` | `1795000000000#b1982cbc…-0000000000000011` |
| `schemaVersion` | S | regla: `topic(4)` | `v1` |
| `eventId` | S | ESP32: 32 hex de arranque + `-` + 16 hex de secuencia | |
| `occurredAt` | N | ESP32 (epoch ms, NTP) | `1795000000000` |
| `receivedAt` | N | regla: `timestamp()` (epoch ms) | `1795000000412` |
| `eventType` | S | ESP32 | `sample`, `occupancy_changed`, `barrier_changed` |
| `origin` | S | ESP32 | `edge` (lecturas); `voz`, `edge`, `nube`, `legado` (barrera) |
| `data` | M | ESP32, validado | lectura: `slots`, `capacidad`, `ocupados`, `disponibles`, `modo`, `distancia_cm`, `vehiculo_detectado` (+ `slot`, `estado_anterior`); barrera: `anterior`, `actual`, `posicion_estimada`, `requestId` |

**Decisiones.** La SK empieza por la hora de ocurrencia con 13 dígitos fijos, así que el orden lexicográfico es el cronológico y un rango de fechas es un `BETWEEN`; el sufijo `#eventId` hace única la clave y permite deduplicar con `attribute_not_exists`. **Índices secundarios:** ninguno; todos los patrones de acceso de esta práctica son por Thing y rango de tiempo. **TTL:** no configurado; con la frecuencia del código la tabla crece ≈ 10 MB/mes por Thing (5.6), muy por debajo de 25 GB, y los reportes necesitan meses de histórico. Se recomienda un atributo `expiresAt` con TTL de 13 meses (7.1).

**Patrones de acceso.**

| # | Patrón | Operación | Clave / condición | Filtro |
|---|---|---|---|---|
| A1 | Guardar un evento sin duplicar | `PutItem` | `thingName`, `eventKey`; `attribute_not_exists(thingName) AND attribute_not_exists(eventKey)` | — |
| A2 | Eventos de un Thing en un rango | `Query` | `thingName = :t AND eventKey BETWEEN :desde13 + "#" AND :hasta13 + "#~"` | — |
| A3 | Transiciones de plazas en un rango (M1, M4) | `Query` A2 | ídem | `eventType = occupancy_changed` (M4: `AND data.slot = :n`) |
| A4 | Aperturas por origen (M2) | `Query` A2 | ídem | `eventType = barrier_changed AND data.actual = abierta` |
| A5 | Tiempo con parqueo lleno / bloqueado (M3) | `Query` A2 | ídem | `eventType IN (occupancy_changed, sample)` |
| A6 | Último estado conocido del objeto | `Query` | `thingName = :t`, `ScanIndexForward = false`, `Limit = 1` | — |
| A7 | Thing vinculado a un usuario | `GetItem` (`SmartParkBindings`) | `userId = :u`, lectura consistente | — |
| A8 | Eventos de todos los Things en un rango (panel multi-parqueo) | requiere índice | **cambio recomendado**: GSI `eventType` + `eventKey` (7.1) | — |

El filtro se aplica después de leer el rango: no reduce el costo de lectura, pero el volumen por Thing es pequeño (≈ 570 ítems/día, 2.10). Para rangos grandes se recorre `LastEvaluatedKey`.

### 2.9 Reglas de AWS IoT y funciones Lambda

**Regla `SmartParkHistoryIngest`** ([`iot-rule.example.json`](../aws/history/iot-rule.example.json)):

```sql
SELECT topic(2) AS thingName, topic(4) AS schemaVersion, eventId, occurredAt, eventType, origin, data,
       timestamp() AS receivedAt
FROM 'smartpark/+/history/v1'
```

| Campo | Valor |
|---|---|
| Tópico | `smartpark/+/history/v1` (el ESP32 publica en `smartpark/smartpark-grupo9-01/history/v1`) |
| Versión SQL | `2016-03-23` |
| Acción | `lambda` → `SmartParkHistoryProcessor` (invocación asíncrona) |
| Permiso | Política basada en recursos de la Lambda: `lambda:InvokeFunction` para `iot.amazonaws.com`, limitada por `SourceArn` (la regla) y `SourceAccount` ([`iot-invoke-permission.example.json`](../aws/history/iot-invoke-permission.example.json)). La acción Lambda no usa rol IAM de regla. |
| Identidad | `thingName` sale del tópico y no del cuerpo; la política del dispositivo solo le permite publicar en su propio tópico (3.2), así que un Thing no puede escribir el histórico de otro. |

La plantilla se crea con `"ruleDisabled": true` para habilitarla solo después de verificar la tabla y la Lambda ([`historial-aws-practica-4.md`](historial-aws-practica-4.md#despliegue-manual-posterior)).

**Funciones Lambda.**

| Lambda | Responsabilidad | Disparador | Recursos | Permisos (rol) |
|---|---|---|---|---|
| `SmartParkAlexaBackend` | Backend de la skill: consultas, comandos, modo/parámetros, vinculación | Alexa Skills Kit (Skill ID) | `SmartParkBindings`, Shadow del Thing | [`alexa-execution-policy.example.json`](../lambda/iam/alexa-execution-policy.example.json) |
| `SmartParkHistoryProcessor` | Validar, enriquecer y almacenar el histórico | Regla `SmartParkHistoryIngest` | `SmartParkHistory`, su grupo de logs | [`processor-execution-policy.example.json`](../aws/history/processor-execution-policy.example.json) |
| `SmartParkPairingAdmin` | Generar el PIN de vinculación | Invocación manual de un operador IAM | `SmartParkBindings` (claves `PAIR#`, `PAIRUSER#`), `DescribeThing` | [`admin-execution-policy.example.json`](../lambda/iam/admin-execution-policy.example.json) |

**Justificación de la separación.** *Responsabilidad única:* cada función tiene un disparador y un propósito. *Frecuencia:* el procesador del histórico se invoca por cada evento (cientos al día) y la skill solo cuando alguien habla; separarlas permite dimensionar memoria y concurrencia por separado. *Permisos:* el procesador solo necesita `dynamodb:PutItem` sobre una tabla y no puede leer vinculaciones ni tocar el Shadow; la skill no puede escribir el histórico; la administración no es invocable por Alexa. *Costo:* el ZIP del procesador es un solo archivo sin el SDK de Alexa (arranque en frío menor que el del backend, que pesa ≈ 18 MB).

### 2.10 Política de reporte

| Canal | Cuándo publica | Límite en el código | Razón |
|---|---|---|---|
| Shadow `update` clase `Control` | Tras ejecutar o rechazar una orden, o aplicar modo/antirrebote | Sin separación mínima; usa la reserva de 200 | El usuario espera la confirmación. |
| Shadow clase `Sensor` | Cambio de plaza, de presencia (3 muestras estables), de validez o Δ distancia ≥ 10 cm | Separación mínima 10 s (`AWS_MIN_PUBLISH_INTERVAL_MS`), salvo presencia | Refleja cambios sin saturar la cuota. |
| Shadow clase `Periodic` | Sin cambios durante 30 s (`AWS_REPORT_INTERVAL_MS`) | Total ordinario ≤ 800/día | Mantiene `reported` dentro de los 180 s que acepta la skill aun perdiendo 5 respaldos seguidos. |
| Shadow `get` (`Recovery`) | Al conectar o reconectar | Usa la reserva | Recupera el `desired` pendiente. |
| Histórico | Cada transición de plaza o barrera + muestra cada 300 s (`HISTORY_SAMPLE_INTERVAL_MS`) | ≤ 1 publicación cada 10 s (`HISTORY_PUBLISH_INTERVAL_MS`), presupuesto separado del Shadow | Los reportes necesitan transiciones exactas; las muestras detectan huecos de cobertura. |
| Sesión de demostración | — | La tarea se desconecta tras 60 min conectados (`AWS_DEMO_MAX_CONNECTED_MS`) hasta reiniciar | Límite académico de consumo. |

El Shadow refleja el **estado actual** (lo que consulta Alexa) y el tópico de histórico guarda la **secuencia de cambios** (lo que necesitan los reportes). Usar `update/accepted` del Shadow como fuente del histórico mezclaría ambos: perdería las transiciones intermedias entre dos reportes y dependería de la cuota del Shadow.

**Mensajes por día (un Thing).** Shadow: como máximo 1000 solicitudes (`AWS_MAX_SHADOW_UPDATES_PER_DAY`), cada una con su respuesta `accepted`. En una sesión de 60 min en reposo: 120 respaldos + eventos. Histórico: 288 muestras/día con conexión continua más las transiciones; con 30 ingresos por plaza al día (240 transiciones) y 40 movimientos de barrera, ≈ 570 eventos/día. Cota física: 8640 publicaciones/día por el intervalo de 10 s. Costo en 5.6.

**Reconexión (base de RNF6).** Tras una falla, el reintento espera 5 s y duplica la espera hasta 300 s (`AWS_RETRY_MAX_MS`), con *jitter* de 0–1 s y un máximo de 10 intentos consecutivos sin un GET válido ([`failedAttempt()`](../src/AwsIoTManager.cpp#L172), [`connectStep()`](../src/AwsIoTManager.cpp#L183)). Los intentos ocurren ≈ 5, 15, 35 y 75 s después de detectar la caída, así que con un corte de 30 s la red vuelve antes del intento de 35 s o del de 75 s: la reconexión queda entre unos segundos y ≈ 50 s después de restaurar la red, según en qué momento se detectó la caída. Un corte mayor a ≈ 20 min agota los 10 intentos y deja al objeto en modo local hasta reiniciar.

### 2.11 Diseño de la skill de Alexa

Nombre de invocación: **`parqueo inteligente`** (es-US). Modelo activo: [`alexa/es-US.smartpark-v2.final.json`](../alexa/es-US.smartpark-v2.final.json).

| Intent | Slot (tipo) | Frases de ejemplo (del modelo) | Acción en el Shadow |
|---|---|---|---|
| `ConsultarDisponiblesIntent` | — | cuántos espacios hay disponibles · consulta los espacios libres · cuántas plazas libres quedan | lee `reported.disponibles`, `capacidad` |
| `ConsultarOcupadosIntent` | — | cuántos espacios están ocupados · consulta los espacios ocupados · cuántas plazas están ocupadas | lee `reported.ocupados`, `capacidad` |
| `ConsultarEstacionamientoIntent` | `espacio` (`AMAZON.NUMBER`, 1–4) | cómo está el estacionamiento {espacio} · está libre la plaza {espacio} · consulta el espacio {espacio} | lee `reported.slots.<n>` |
| `ConsultarModoIntent` | — | consulta el modo del parqueo · en qué modo está el parqueo · cuál es el modo actual | lee `reported.modo` |
| `ActivarModoAutomaticoIntent` | — | activa el modo automático · pon el parqueo en modo automático · cambia a modo automático | `desired.modo = "automatico"` |
| `ActivarModoManualIntent` | — | activa el modo manual · pon el parqueo en modo manual · cambia a modo manual | `desired.modo = "manual"` |
| `ConsultarAntirreboteIntent` | — | consulta el antirrebote · cuántos milisegundos tiene el antirrebote · cuál es el valor del antirrebote | lee `reported.parametros_auto.antirrebote_ir_ms` |
| `CambiarAntirreboteIntent` | `milisegundos` (`AMAZON.NUMBER`, 50–1000) | ajusta el antirrebote a {milisegundos} milisegundos · cambia el antirrebote a {milisegundos} · configura {milisegundos} milisegundos de antirrebote | `desired.parametros_auto.antirrebote_ir_ms` |
| `AbrirBarreraIntent` / `CerrarBarreraIntent` | — | abre la barrera · levanta la barrera · abrir … / cierra la barrera · baja la barrera · cerrar … (7 frases cada uno) | `desired.barrera` |
| `EstadoBarreraIntent` | — | consulta el estado de la barrera · dime el estado de la barrera … | lee `reported.barrera` |
| `ConsultarDistanciaIntent` / `ConsultarVehiculoIntent` | — | consulta la distancia … / hay un auto esperando … | lee `distancia_cm` / `vehiculo_detectado` |
| `VincularParqueoIntent` / `CambiarVinculacionIntent` | `codigo` (`AMAZON.FOUR_DIGIT_NUMBER`) | vincula mi parqueo con el codigo {codigo} … / cambia mi vinculacion con el código {codigo} … | `SmartParkBindings` |
| `DesvincularParqueoIntent` | — | desvincula mi parqueo · elimina mi vinculacion … | `SmartParkBindings` |
| `AMAZON.HelpIntent`, `CancelIntent`, `StopIntent`, `NavigateHomeIntent` | — | integrados | — |

Los valores fuera de rango del slot se rechazan en la Lambda sin escribir el Shadow ([`number_slot()`](../lambda/handlers/parking_handlers.py#L9)). Las respuestas de los cambios dicen *"Solicité…; el dispositivo aún debe confirmarlo"*, porque la escritura en `desired` no garantiza su aplicación.

### 2.12 Diseño de reportes (mockups)

Cuatro reportes para el administrador del parqueo. Los valores dibujados son ilustrativos; lo que se diseña es **qué** se muestra y **de qué consulta** sale. Todos parten del patrón A2 (2.8) sobre `SmartParkHistory`. Fuentes editables `.excalidraw` y exportaciones SVG/PNG en [`docs/mockups/`](mockups/).

**M1 — Ocupación por plaza y franja horaria.** Decisión: horarios de mayor demanda (personal, tarifas por hora).

![M1 Ocupación por plaza y franja horaria](mockups/M1-ocupacion-por-plaza.png)

| Elemento | Atributos | Consulta |
|---|---|---|
| Mapa de calor plaza × hora | `occurredAt`, `data.slot`, `data.estado_anterior`, `data.slots` | `Query` `SmartParkHistory`: `thingName = :t AND eventKey BETWEEN :desde13# AND :hasta13#~`; filtro `eventType IN (occupancy_changed, sample)`; tiempo ocupado = diferencias de `occurredAt` entre transiciones, repartidas por hora |
| Ocupación media, hora pico, plaza más usada | derivados del mapa | misma consulta |
| Cobertura | intervalos con `desconocido` o sin muestras de 300 s | misma consulta, `eventType = sample` |

**M2 — Aperturas de barrera por origen.** Decisión: cuánto se usa la voz frente a la operación local.

![M2 Aperturas de barrera por origen](mockups/M2-aperturas-por-origen.png)

| Elemento | Atributos | Consulta |
|---|---|---|
| Barras apiladas por día y origen | `occurredAt`, `origin`, `data.actual` | `Query` A2; filtro `eventType = barrier_changed AND data.actual = abierta`; agrupar por día y `origin` |
| KPIs (total, % por origen) | `origin` | misma consulta |

En la implementación actual las órdenes de la skill usan el formato V1 (`desired.barrera`) y se registran con `origin = legado`; el origen `voz` lo produce el formato V2 `orden_barrera`, que la skill aún no emite (5.5). La serie `nube` queda en cero mientras no existan reglas de Nube.

**M3 — Saturación: parqueo lleno y aperturas bloqueadas.** Decisión: si la capacidad es insuficiente.

![M3 Saturación](mockups/M3-saturacion.png)

| Elemento | Atributos | Consulta |
|---|---|---|
| Minutos por día con parqueo lleno | `occurredAt`, `data.disponibles`, `data.capacidad` | `Query` A2; filtro `eventType IN (occupancy_changed, sample)`; lleno = `disponibles = 0` y `ocupados = capacidad` |
| Minutos con apertura bloqueada | + `data.modo` | misma consulta; bloqueado = lleno y `modo = automatico` |
| Episodios de lleno | transiciones a `disponibles = 0` | misma consulta |
| Aperturas rechazadas | — | **Cambio recomendado:** el firmware no publica las órdenes bloqueadas (solo `reported.ultima_orden`). Requiere un evento nuevo `order_rejected` en el histórico y su validación en la Lambda (7.1). |

**M4 — Tiempo de permanencia por plaza.** Decisión: rotación de plazas y tiempo máximo de estadía.

![M4 Permanencia por plaza](mockups/M4-permanencia.png)

| Elemento | Atributos | Consulta |
|---|---|---|
| Permanencia media por plaza, mediana, P95 | `occurredAt`, `data.slot`, `data.estado_anterior` | `Query` A2; filtro `eventType = occupancy_changed AND data.slot = :n`; permanencia = `occurredAt` (ocupado → libre) − `occurredAt` (libre → ocupado) |
| Últimas estancias | ídem | misma consulta con `ScanIndexForward = false` |
| Estancias y rotación | conteo de pares | misma consulta |

Ningún mockup requiere un índice nuevo para un Thing; un panel que compare varios Things necesita el GSI de A8.

---

## 3. Desarrollo e Implementación

### 3.1 Estructura del código

**Firmware (C++, PlatformIO, `esp32doit-devkit-v1`, Arduino; bibliotecas fijadas en [`platformio.ini`](../platformio.ini): ESP32Servo 1.1.2, PubSubClient 2.8, ArduinoJson 6.21.5).**

| Archivo | Responsabilidad |
|---|---|
| [`src/main.cpp`](../src/main.cpp) | `setup()`/`loop()`: orden de ejecución, comandos Serial y regla E2 para órdenes Serial |
| [`include/Parking.h`](../include/Parking.h), [`src/Parking.cpp`](../src/Parking.cpp) | IR, antirrebote, conteos, LEDs, modo (reglas E1 y E2) |
| [`src/BarrierServo.cpp`](../src/BarrierServo.cpp) | SG90, PWM perezoso y posición estimada por tiempo |
| [`src/UltrasonicSensor.cpp`](../src/UltrasonicSensor.cpp) | HC-SR04 con espera acotada e histéresis de presencia |
| [`src/ParkingDisplay.cpp`](../src/ParkingDisplay.cpp) | LCD 20×4 por PCF8574, sin bloqueos: una fila por llamada |
| [`include/SerialCommands.h`](../include/SerialCommands.h) | Analizador acotado de `OPEN`, `CLOSE`, `STATUS`, `DISTANCE`, `SLOT1`, `HELP` |
| [`src/AwsIoTManager.cpp`](../src/AwsIoTManager.cpp) | Tarea de red: Wi-Fi, NTP, MQTT/TLS, Shadow, cuota, órdenes |
| [`include/ShadowBudget.h`](../include/ShadowBudget.h) | Cuota diaria persistente por clase de solicitud |
| [`src/HistoryPublisher.cpp`](../src/HistoryPublisher.cpp) | Colas y publicación del histórico |
| [`include/config.example.h`](../include/config.example.h), [`include/secrets.example.h`](../include/secrets.example.h) | Plantillas de configuración sin secretos |

**Fragmentos clave.**

- **Tarea FreeRTOS.** [`AwsIoTManager::begin()`](../src/AwsIoTManager.cpp#L27) crea `smartpark-aws` (pila de 12 288 B, prioridad 1) y tres colas de un elemento. `loop()` publica su instantánea con `xQueueOverwrite` y nunca espera a la red: si TLS tarda, se pierde una instantánea intermedia, no el control local.
- **Antirrebote de los IR.** [`Parking::update()`](../src/Parking.cpp#L27) muestrea cada 10 ms; una lectura nueva reinicia `candidateSince_` y el estado solo cambia cuando el candidato se mantiene `debounceMs`. Si `loop()` se atrasa más de 3 muestras, la ventana se reinicia (`interrupted`) para no confirmar con datos viejos. Cambiar el antirrebote reinicia los candidatos ([`configure()`](../src/Parking.cpp#L18)).
- **Regla de bloqueo.** Una sola expresión, [`openingBlocked()`](../include/Parking.h#L15): `mode == Automatic && available == 0`. La usan la orden remota ([`applyPending()`](../src/AwsIoTManager.cpp#L42)), la Serial ([`main.cpp`](../src/main.cpp#L62)), la LCD y `reported.bloqueo_apertura`, de modo que no hay dos versiones de la regla.
- **Manejo del Shadow.** [`receive()`](../src/AwsIoTManager.cpp#L329) filtra el JSON con un `StaticJsonDocument` de 2048 B, ignora deltas con `version` vieja y respuestas sin el `clientToken` propio. [`applyDesired()`](../src/AwsIoTManager.cpp#L241) valida modo y antirrebote, los guarda en NVS **antes** de aplicarlos y registra la orden en NVS para no repetirla tras un reinicio.
- **Cuota de solicitudes.** [`reserve()`](../src/AwsIoTManager.cpp#L160) cuenta cada intento en NVS antes de publicar; [`ShadowBudget::allows()`](../include/ShadowBudget.h#L22) deja 200 de las 1000 diarias para `Control` y `Recovery`. El día UTC solo avanza: un reinicio no repone la cuota.
- **Publicador de histórico.** [`enqueue()`](../src/HistoryPublisher.cpp#L34) exige hora UTC válida y usa `xQueueSend` sin espera: con la cola llena descarta el evento nuevo y lo cuenta. [`publish()`](../src/HistoryPublisher.cpp#L135) envía uno cada ≥ 10 s, con prioridad barrera > plazas > muestra y hasta 3 intentos con los mismos bytes (mismo `eventId` y `occurredAt`, por lo que un reintento no duplica filas).

**Lambdas (Python 3.13, estilo funcional por responsabilidad).**

| Paquete | Archivos | Responsabilidad |
|---|---|---|
| `SmartParkAlexaBackend` | [`lambda_function.py`](../lambda/lambda_function.py), [`handlers/`](../lambda/handlers/), [`services/`](../lambda/services/) | Validación del sobre de Alexa, intents, Shadow y vinculación |
| `SmartParkHistoryProcessor` | [`history_lambda_function.py`](../lambda/history_lambda_function.py) | Validación y `PutItem` condicional |
| `SmartParkPairingAdmin` | [`admin_lambda_function.py`](../lambda/admin_lambda_function.py), [`administration/`](../lambda/administration/) | Generación de PIN |

- **Validación y enriquecimiento del histórico.** [`validate_event()`](../lambda/history_lambda_function.py#L87) exige `schemaVersion = "v1"`, `thingName` y `eventId` con patrón, `receivedAt` entero y `occurredAt` no más de 300 s por delante de `receivedAt`. [`validate_snapshot()`](../lambda/history_lambda_function.py#L39) exige exactamente las claves esperadas, plazas 1..`capacidad`, conteos coherentes con `slots`, distancia 2–400 cm y presencia solo con distancia; [`validate_barrier()`](../lambda/history_lambda_function.py#L74) exige una transición real. Un inválido se registra como `history_invalid field=<campo>` y devuelve `invalid` sin reintento; un error de DynamoDB se propaga para que la invocación asíncrona reintente.
- **Handlers de modo y parámetros.** [`parking_handlers.py`](../lambda/handlers/parking_handlers.py): las consultas leen `reported` con control de antigüedad por atributo; los cambios llaman a [`set_mode`/`set_debounce`](../lambda/services/shadow_service.py#L81), que escriben un parche mínimo en `desired` sin leer antes el Shadow. Los límites 50–1000 están declarados junto a la nota de sincronía con el firmware ([`shadow_service.py`](../lambda/services/shadow_service.py#L11)).
- **Sin credenciales.** Los clientes de boto3 se crean en la primera invocación y usan el rol de ejecución; ningún archivo contiene *access keys*. Una sola llamada por operación (`total_max_attempts: 1`) con tiempos de 0,5 s de conexión y 1 s de lectura ([`runtime.py`](../lambda/services/runtime.py)).

### 3.2 Configuraciones en AWS y permisos asignados

Las plantillas usan `<REGION>`/`<ACCOUNT_ID>` ([`aws/history/`](../aws/history/)) o `us-east-1`/`REEMPLAZAR_ACCOUNT_ID` ([`lambda/iam/`](../lambda/iam/), [`docs/aws-iot-policy.example.json`](aws-iot-policy.example.json)).

| Recurso | Configuración |
|---|---|
| Thing | `smartpark-grupo9-01` (`THING_NAME`); `clientId` MQTT = nombre del Thing |
| Certificados | X.509 del Thing y Amazon Root CA 1 en `include/secrets.h`, excluido por [`.gitignore`](../.gitignore); plantilla vacía en [`secrets.example.h`](../include/secrets.example.h). Se quedan fuera del repositorio porque quien tenga la clave privada puede suplantar al objeto. |
| Política IoT del dispositivo | [`aws-iot-policy.example.json`](aws-iot-policy.example.json) (Shadow) + [`device-publish-policy.example.json`](../aws/history/device-publish-policy.example.json) (histórico) |
| Regla | `SmartParkHistoryIngest` (2.9) |
| Lambda `SmartParkHistoryProcessor` | Python 3.13, handler `history_lambda_function.lambda_handler`, 128 MB (punto de partida), `HISTORY_TABLE_NAME=SmartParkHistory` ([`historial-aws-practica-4.md`](historial-aws-practica-4.md#despliegue-manual-posterior)) |
| Lambda `SmartParkAlexaBackend` | Python 3.13, handler `lambda_function.lambda_handler`, disparador Alexa Skills Kit con Skill ID; variables `ALEXA_SKILL_ID`, `TABLE_NAME=SmartParkBindings`, `IOT_DATA_ENDPOINT` (opcional), `MAX_REPORTED_AGE_SECONDS` (opcional, 180 por defecto) |
| Lambda `SmartParkPairingAdmin` | Python 3.13, handler `admin_lambda_function.lambda_handler`, sin disparador; `TABLE_NAME=SmartParkBindings` |
| Tablas | `SmartParkBindings` (PK `userId`), `SmartParkHistory` ([`table.example.json`](../aws/history/table.example.json)) |
| Roles | Confianza `lambda.amazonaws.com` ([`execution-role-trust.example.json`](../lambda/iam/execution-role-trust.example.json)) + una política por Lambda |
| Logs | `/aws/lambda/SmartParkHistoryProcessor` creado de antemano con retención (por ejemplo 14 días) |

`SmartParkAlexaBackend` y `SmartParkPairingAdmin` se configuran con x86_64, 128 MB y *timeout* de 10 s ([`lambda/README.md`](../lambda/README.md#L122)); la captura de la consola en la sesión de despliegue confirma esos valores (8.4).

**Permisos.**

| Principal | Permiso asignado | Motivo | Mínimo en producción |
|---|---|---|---|
| Certificado del ESP32 | `iot:Connect` sobre `client/smartpark-grupo9-01` | Solo ese `clientId` | Igual, con `${iot:Connection.Thing.ThingName}` para no repetir la política por Thing |
| Certificado del ESP32 | `iot:Publish` en `shadow/get`, `shadow/update` del Thing | Pedir y reportar el Shadow | Igual |
| Certificado del ESP32 | `iot:Subscribe`/`iot:Receive` en los 5 tópicos de respuesta del Thing | Recibir `delta` y acuses | Igual |
| Certificado del ESP32 | `iot:Publish` en `smartpark/${iot:Connection.Thing.ThingName}/history/v1` con `iot:Connection.Thing.IsAttached = true` | Cada objeto solo escribe su histórico | Ya es mínimo |
| Regla `SmartParkHistoryIngest` | `lambda:InvokeFunction` en la política de recursos de la Lambda, con `SourceArn` y `SourceAccount` | Solo esa regla puede invocar el procesador | Ya es mínimo |
| `SmartParkHistoryProcessor` | `dynamodb:PutItem` en `table/SmartParkHistory`; `logs:CreateLogStream`, `logs:PutLogEvents` en su grupo | Escribir eventos y sus logs | Ya es mínimo (no necesita `logs:CreateLogGroup` porque el grupo se crea antes) |
| `SmartParkAlexaBackend` | `dynamodb:GetItem/PutItem/DeleteItem/UpdateItem` en `SmartParkBindings` con `dynamodb:LeadingKeys` por prefijo | Vincular sin poder tocar registros de otro tipo | Igual |
| `SmartParkAlexaBackend` | `iot:DescribeThing`, `iot:GetThingShadow`, `iot:UpdateThingShadow` en `thing/<Thing autorizado>` | Leer y escribir solo el Shadow de los Things registrados | Igual, con una entrada por Thing o etiquetas (`aws:ResourceTag`) |
| `SmartParkAlexaBackend` | `iot:DescribeEndpoint` en `*` | La acción no admite ARN de recurso | Eliminarla configurando `IOT_DATA_ENDPOINT` |
| `SmartParkPairingAdmin` | `dynamodb:PutItem` con `LeadingKeys` `PAIR#*`, `PAIRUSER#*`; `iot:DescribeThing` | Crear la ventana del PIN | Ya es mínimo |
| Operador | `lambda:InvokeFunction` sobre `SmartParkPairingAdmin` ([`admin-invoke-policy.example.json`](../lambda/iam/admin-invoke-policy.example.json)) | Solo un humano autorizado genera PIN | Ya es mínimo |

**Comodines.** El único `*` de recurso es `iot:DescribeEndpoint`, justificado arriba. No hay `iot:*` ni políticas administradas por AWS en las plantillas.

**Alerta de facturación.** No hay evidencia de una alerta de facturación en el repositorio; se recomienda un presupuesto de AWS Budgets de 1 USD/mes con aviso por correo (7.3).

### 3.3 Configuraciones en Alexa

| Elemento | Valor |
|---|---|
| Idioma | Spanish (US), `es-US` |
| Invocación | `parqueo inteligente` |
| Modelo de interacción | [`alexa/es-US.smartpark-v2.final.json`](../alexa/es-US.smartpark-v2.final.json) (modelo combinado P3 + RF14; se importa en **Build → JSON Editor**) |
| Endpoint | AWS Lambda ARN de `SmartParkAlexaBackend`, región `us-east-1` |
| Vinculación con la Lambda | Disparador Alexa Skills Kit con verificación de Skill ID en la Lambda y en el código (`ALEXA_SKILL_ID`, [`lambda_function.py`](../lambda/lambda_function.py#L39)) |
| Prueba | **Test → Development** en el simulador |

Procedimiento de despliegue y validación en consola: [`alexa-practica-4.md`](alexa-practica-4.md). El repositorio conserva también [`alexa/es-US.interaction-model.json`](../alexa/es-US.interaction-model.json), que es el modelo que verifica la prueba automática (5.5).

---

## 4. Pruebas y Validaciones

### 4.1 Plan de pruebas y hojas de registro

**Enfoque.** Tres niveles: pruebas automáticas con dobles (4.5), pruebas funcionales en vivo (una por RF como mínimo, 4.2) y pruebas no funcionales con valores medidos frente a lo declarado en 1.4 (4.3). Si un resultado no cumple, se aplica 4.4 y se vuelve a medir.

**Entorno.** ESP32 DOIT DevKit V1 con el circuito de 2.3; firmware del commit evaluado; AWS `us-east-1` con los recursos de 3.2; simulador de Alexa en **Test → Development**; un PC con PlatformIO; un celular que grabe a ≥ 30 fps; cinta métrica.

**Orígenes de tiempo.**

| Código | Fuente | Resolución |
|---|---|---|
| S | Monitor serie con marca del PC: `pio device monitor -f time` (PC sincronizado por NTP) | 1 ms (llegada al PC) |
| V | Video ≥ 30 fps contado por cuadros | 33 ms a 30 fps |
| R | `receivedAt` del ítem (`timestamp()` de la regla) | 1 ms |
| O | `occurredAt` del ítem (reloj NTP del ESP32) | 1 ms |
| C | Hora de `REPORT`/`END` en CloudWatch Logs (UTC) | 1 ms |

**Criterios de aceptación.** Una prueba funcional se aprueba si el valor observado coincide con el esperado en todas sus repeticiones. Una no funcional, si el indicador cumple el valor de 1.4.

**Hojas de registro** (imprimir antes de medir): [`hoja-pruebas-funcionales.md`](evidencias/hojas-registro/hoja-pruebas-funcionales.md) y [`hoja-pruebas-no-funcionales.md`](evidencias/hojas-registro/hoja-pruebas-no-funcionales.md). La evidencia de cada prueba se guarda en `docs/evidencias/pruebas/<ID>/`.

**Orden de ejecución (sesión de ≈ 2 h, reutilizando montajes).**

| Bloque | Pruebas | Montaje y grabación compartidos | Tiempo |
|---|---|---|---|
| 0 | Preparación | Despliegue según 3.2; habilitar la regla; PC con `pio device monitor -f time`; cliente MQTT suscrito a `$aws/things/smartpark-grupo9-01/shadow/update/accepted` y a `smartpark/+/history/v1`; consola de DynamoDB y CloudWatch abiertas | 15 min |
| 1 | PNF-01 (se mide durante los bloques 2–4), PF-01, PNF-02, PNF-03, PF-11, PNF-05 | Cámara fija sobre las plazas y los LEDs | 30 min |
| 2 | PF-04, PF-05, PF-03, PF-15, PF-02, PF-12, PF-13, PF-16, PNF-04 | Cámara fija con la pantalla del simulador y el servo en el mismo cuadro | 30 min |
| 3 | PNF-07 (ventana de 10 min) + PF-06, PF-07, PF-10, PNF-06, PNF-08 | Consola de DynamoDB y CloudWatch | 15 min |
| 4 | PF-08, PF-09 | Cliente MQTT de prueba | 5 min |
| 5 | PNF-09, PF-14, PNF-10 | Router con el monitor serie visible | 15 min |
| 6 | PF-17 y exportación de muestra de DynamoDB | Consola de DynamoDB | 10 min |

### 4.2 Pruebas funcionales

"Resultado obtenido", "Evidencia" y "Estado" se completan con la hoja de registro.

| ID | RF | Procedimiento | Resultado esperado | Resultado obtenido | Evidencia | Estado |
|---|---|---|---|---|---|---|
| PF-01 | RF1, RF2 | Enviar `DISTANCE` y `SLOT1`; cubrir un IR; observar el cliente MQTT. | Distancia y plaza correctas en serie; `update/accepted` con `slots`, `ocupados`, `disponibles`, `distancia_cm`. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-02 | RF3, RF5 | En `manual`: *"abre la barrera"*, luego *"cierra la barrera"*. | `desired.barrera` cambia; el servo se mueve una vez; `reported.barrera` igual al pedido; `ultima_orden.resultado = aceptada`; sin `delta` pendiente. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-03 | RF4, RF6 | *"consulta la distancia"*, *"hay un auto esperando"*, *"consulta el estado de la barrera"*; repetir con el ESP32 apagado > 180 s. | Respuestas iguales a `reported`; con dato viejo: *"…está desactualizado…"*. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-04 | RF7, RF8 | Abrir la skill sin vinculación; generar PIN con `SmartParkPairingAdmin`; *"vincula mi parqueo con el codigo …"*; cerrar y reabrir. | Sin vinculación: instrucción de vincular. Con PIN: fila `userId → smartpark-grupo9-01` en `SmartParkBindings` y bienvenida de usuario vinculado. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-05 | RF9 | Generar PIN para un Thing inexistente; *"cambia mi vinculacion con el código …"*; *"desvincula mi parqueo"*. | PIN rechazado (*"El Thing solicitado no existe."*); cambio con revisión nueva; fila eliminada. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-06 | RF10 | Cubrir y liberar una plaza; ver CloudWatch del procesador. | Una invocación (`START`/`END`/`REPORT`) por mensaje publicado. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-07 | RF11 | Consulta en DynamoDB: `thingName = smartpark-grupo9-01` y `eventKey` entre los últimos 15 min; exportar a CSV. | Ítems `sample` y `occupancy_changed` con `thingName`, `schemaVersion`, `receivedAt`, `data` coherente. CSV en `docs/evidencias/pruebas/PF-07/`. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-08 | RF11 | Desde el cliente MQTT, publicar en `smartpark/smartpark-grupo9-01/history/v1` 4 inválidos: `distancia_cm: 999`, `capacidad: 5`, `eventType: "door_opened"`, `origin: "voz"` en `sample`. | 4 líneas `history_invalid field=…` en CloudWatch; 0 ítems nuevos. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-09 | RF11 | Publicar dos veces el mismo evento válido (mismo `eventId` y `occurredAt`). | Un solo ítem. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-10 | RF12 | Abrir por voz, cerrar por voz, abrir con `OPEN` y cerrar con `CLOSE`. | 4 ítems `barrier_changed` con `anterior`/`actual` correctos; `origin = legado` para voz (formato V1, 5.5) y `edge` para Serial. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-11 | RF13 (E1) | Cubrir y liberar cada plaza. | LED rojo al ocupar, verde al liberar; LCD y `reported` actualizados; ítem `occupancy_changed` por transición. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-12 | RF13 (E2) | *"activa el modo automático"*; ocupar las 4 plazas; *"abre la barrera"* y luego `OPEN` por Serial. | Barrera inmóvil; `ultima_orden.resultado = bloqueada`; LCD `APERTURA BLOQUEADA`; `bloqueo_apertura: true`; Serial *"Apertura bloqueada: sin plazas confirmadas"*; sin ítem `barrier_changed`. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-13 | RF13 (E2, E3) | Con el parqueo lleno en `automatico`: *"cierra la barrera"*; liberar una plaza y *"abre la barrera"*; pasar a `manual` con el parqueo lleno y *"abre la barrera"*. | Cierre aceptado; apertura aceptada al liberar una plaza; en `manual` apertura aceptada aunque esté lleno. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-14 | RF13 (E4) | Desconectar la red; *"abre la barrera"*; restablecer después de 60 s. | La barrera no se mueve al reconectar (orden con más de 30 s). | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-15 | RF14 | Probar en el simulador cada frase de la hoja (24 frases de 8 intents + 4 valores límite: 49, 1000, 1001 y plaza 5), revisando **JSON Input**, la respuesta y el Shadow. | Intent y slot correctos; consultas iguales a `reported`; cambios en `desired` y luego en `reported`; valores fuera de rango rechazados sin escribir el Shadow. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-16 | RF14, convivencia | En `automatico` con 4 plazas ocupadas: *"abre la barrera"*, *"consulta el estado de la barrera"*, *"en qué modo está el parqueo"*, *"activa el modo manual"*, *"abre la barrera"*. | Primer pedido bloqueado y barrera `cerrada`; modo `automático`; tras pasar a manual, apertura aceptada. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PF-17 | RF15 | Ejecutar en la consola de DynamoDB las consultas de M1–M4 (2.12) sobre los datos de la sesión. | Cada consulta devuelve los ítems y atributos necesarios para su mockup. | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |

**Multiusuario (RF7–RF9).** PF-04 se repite con una segunda cuenta de desarrollador vinculada a otro Thing si está disponible; la separación por usuario también la cubren `test_v2_writes_precise_patches_and_uses_each_users_binding` y las pruebas de aislamiento (4.5).

### 4.3 Pruebas no funcionales

| ID | RNF | Procedimiento | Rep. | Indicador | Criterio (1.4) | Resultado obtenido | Evidencia | Estado |
|---|---|---|---|---|---|---|---|---|
| PNF-01 | RNF1 | Conectado durante los bloques 1–4 (≥ 15 min) con uso normal; registrar `STATUS` al inicio y al fin. | 1 | reinicios, bloqueos, reconexiones | 0 / 0 / 0 | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PNF-02 | RNF2 | Objeto plano a 5, 10, 15, 20, 30 y 50 cm; 5 lecturas `DISTANCE` por distancia. | 30 | e = max\|dᵢ − d_real\| | ≤ 1 cm | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PNF-03 | RNF3 | 10 entradas y 10 salidas por plaza con un vehículo de maqueta. | 80 | aciertos / 80 × 100 | ≥ 95 % | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PNF-04 | RNF4 | Video con simulador y servo en cuadro; 5 *"abre"* y 5 *"cierra"*; desde el último cuadro de la frase hasta el cuadro en que la barrera se detiene. | 10 | t = Δcuadros / fps; máximo y promedio | máx ≤ 3,0 s | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PNF-05 | RNF9 | Video del IR y su LED: desde el cuadro en que el vehículo cubre el IR hasta el cambio del LED (7 rep.); desde que la 4.ª plaza queda cubierta hasta `APERTURA BLOQUEADA` en la LCD (3 rep.). | 10 | t = Δcuadros / fps | máx ≤ 0,3 s | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PNF-06 | RNF8 | Para ≥ 20 eventos: `occurredAt` (O) y `receivedAt` (R) del ítem; hora de `REPORT` (C) de la invocación correspondiente (los eventos están separados ≥ 10 s, así que se emparejan por orden). Δ1 = C − R mide regla → invocación asíncrona → validación → `PutItem`; Δ0 = R − O mide la cola del ESP32, la red y el desfase NTP. | ≥ 20 | mín, promedio, máx y P95 de Δ1 y Δ0 | Δ1 máx ≤ 5 s; Δ0 máx ≤ 15 s | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PNF-07 | RNF7 | Ventana de 10 min con uso normal: N = diferencia de `Historico: MQTT local` (serie, cada 30 s) entre inicio y fin; M = ítems con `receivedAt` en la ventana (+5 s). Además, los 4 inválidos de PF-08. | 1 ventana | M / N × 100; inválidos descartados / 4 | ≥ 99 %; 4/4 | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PNF-08 | RNF5 | 5 min en reposo con el cliente MQTT suscrito a `update/accepted`; 15 min de ítems `sample`. | 1 | intervalo medio entre `update/accepted`; muestras cada ≈ 300 s | 30 s ± 1 s; 3 muestras en 15 min | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PNF-09 | RNF6 | Apagar el punto de acceso 30 s (cronometrado con S); durante el corte, *"activa el modo manual"* (o automático); encender. Medir hasta `AWS MQTT/TLS conectado` y hasta el `update/accepted` con el nuevo `modo`. | 3 | t_rec = conectado − encendido; t_sinc = reported − encendido | t_rec ≤ 60 s; t_sinc ≤ 70 s | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| PNF-10 | RNF10 | Corte de 2 min: ocupar/liberar plazas, probar E2 con `OPEN` y `CLOSE`; contar transiciones; tras reconectar, contar ítems y tiempo de drenado. | 1 | reglas operativas; ítems / transiciones | E1–E3 operan; ítems = transiciones si ≤ 16 de plazas y ≤ 8 de barrera | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |
| — | RNF9 (Nube) | No aplica: no hay reglas de control en la Nube (2.6). | — | — | — | No aplica | — | No aplica |
| — | RNF11 | Cálculo (5.6). | — | — | — | — | — | — |

**Fórmulas.** Promedio x̄ = Σxᵢ/n; P95 = valor en la posición ⌈0,95·n⌉ de la muestra ordenada (solo con n ≥ 20); integridad = M/N·100.

**Operación sin conexión según el código.** Siguen activos E1–E4, LEDs, LCD, comandos Serial y el modo guardado en NVS. Los eventos del histórico se siguen capturando si el reloj ya se sincronizó antes del corte, en colas de 8 (barrera), 16 (plazas) y 1 (muestra); con la cola llena se descarta el evento nuevo (`cola_llena`). Al volver MQTT y sincronizar el Shadow se drenan a razón de uno cada 10 s (25 eventos como máximo ≈ 250 s). El Shadow no encola estados intermedios: al reconectar publica el estado actual. Un reinicio pierde las colas ([`historial-aws-practica-4.md`](historial-aws-practica-4.md#publicador-integrado-en-el-firmware)).

### 4.4 Análisis de fallas y correcciones

Formato para cada prueba que no cumpla. El diagnóstico usa `STATUS` (contadores de Shadow, histórico, NVS, hora y reintentos), los logs de CloudWatch y la tensión del riel del servo.

| ID prueba | Síntoma | Datos de diagnóstico | Causa probable | Acción correctiva | Nueva medición | Estado |
|---|---|---|---|---|---|---|
| Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución |

Guía de diagnóstico preparada a partir del código:

| Síntoma | Qué revisar | Causa probable |
|---|---|---|
| Reinicio (reaparece el banner) | Tensión de 5 V al mover el servo; causa de reinicio (el firmware no la imprime hoy, 5.5) | Caída de tensión por el SG90 en la misma fuente; *brownout* |
| `reported` no se actualiza | `Presupuesto ordinario=agotado`, `UPDATE rechazado` en serie | Cuota de 800 ordinarias agotada; política IoT incorrecta |
| `MQTT local` sube pero no hay ítems | Regla deshabilitada; `history_invalid` en CloudWatch; `Errors` de la Lambda | Regla en `ruleDisabled: true`; esquema distinto; permiso de invocación |
| `sin_hora` crece | `horaUTC=INVALIDA` | NTP sin respuesta |
| La barrera no responde a la voz tras un corte | `ultima_orden` ausente | Orden V1 de más de 30 s (E4, comportamiento esperado) |

### 4.5 Pruebas automáticas

Ejecutadas el 2026-11-21: PA-03 sobre el commit `876be` de la rama `docs/report-generation`; PA-01 y PA-02 en Windows 11 con PlatformIO y MSYS2 (g++ 16.2.0), con `include/config.h` e `include/secrets.h` copiados de las plantillas. Salidas completas, con fecha y comando, en [`docs/evidencias/pruebas-automaticas/`](evidencias/pruebas-automaticas/).

| ID | Suite | Comando | Pruebas | Resultado | Evidencia |
|---|---|---|---|---|---|
| PA-01 | Compilación del firmware | `pio run` | — | **SUCCESS** en 30,4 s (`espressif32@7.1.0`, `toolchain-xtensa-esp32` 8.4.0): compila las 7 unidades de `src/`, enlaza `firmware.elf` y genera `firmware.bin`. **RAM 49 960 / 327 680 B (15,2 %)**; **Flash 964 425 / 1 310 720 B (73,6 %)**. | [`pio-run.txt`](evidencias/pruebas-automaticas/pio-run.txt) |
| PA-02 | Firmware con dobles (host) + contrato del histórico | `SMARTPARK_HISTORY_PYTHON=lambda/.venv/Scripts/python.exe sh test/run_host_tests.sh` | 5 suites + contrato | **5/5 PASS + contrato PASS**: IR, rebotes, conteos, LEDs y LCD con fallo I²C; sensor, histéresis, servo, parser y `loop`; AWS con dobles (GET, comandos, `reported`, versiones, cuota/NVS, *backoff*, demo); modos, lleno y deduplicación V1/V2 tras reconexión o reinicio; publicación del histórico (reloj, prioridad, reintentos, desborde). Tamaños medidos: Shadow reporte/GET/delta/accepted 476/2458/672/1887 B, GET MQTT 2518 B; histórico máx. 347 B (MQTT 394 B). Contrato firmware → Lambda ([`check_history_contract.py`](../test/check_history_contract.py)): **12 cargas MQTT generadas por el firmware aceptadas por `SmartParkHistoryProcessor`**, y el reintento conserva la misma clave (`eventKey`). | [`host-tests.txt`](evidencias/pruebas-automaticas/host-tests.txt) |
| PA-03 | Lambdas | `AWS_EC2_METADATA_DISABLED=true python -m unittest discover -s tests` (en `lambda/`) | 102 | **102 OK** (con `PYTHONUTF8=1`; sin esa variable, Windows cp1252 provoca 1 error de lectura del modelo de Alexa) | [`lambda-unittest.txt`](evidencias/pruebas-automaticas/lambda-unittest.txt) |

Cobertura de PA-03: vinculación y PIN (`test_admin`, `test_pin_public`, `test_isolation`), backend de la skill incluido RF14 (`test_backend`, `test_v2_*`), sesión (`test_session`) e histórico (`test_history`: lectura y capacidad reducida, orígenes y transiciones de barrera, inválidos sin inicializar el SDK, duplicado y reintento, coherencia de plantillas). PA-02 cubre `Parking`, la LCD, la cuota, la reconexión con `desired` pendiente, la prioridad de control, la publicación del histórico y el contrato firmware → Lambda ([`test/README`](../test/README)). Los tamaños de PA-02 confirman que el GET del Shadow cabe en `MQTT_BUFFER_SIZE = 3072` y que cada mensaje se factura como uno solo en IoT Core (< 5 KB) y cada ítem del histórico como 1 WRU en DynamoDB (< 1 KB) (5.6).

---

## 5. Resultados

### 5.1 Resumen de cumplimiento

**Pruebas automáticas (4.5):** PA-01 compila y enlaza el firmware (RAM 15,2 %, Flash 73,6 %); PA-02 pasa 5/5 suites con dobles y el contrato firmware → Lambda (12 cargas aceptadas); PA-03 pasa 102/102 pruebas de las Lambdas. Las pruebas en vivo de cada RF se ejecutan en la sesión de 4.1.

| RF | Prueba en vivo | Prueba automática | Resultado | Estado |
|---|---|---|---|---|
| RF1 | PF-01, PF-11 | PA-02 (PASS) | Pendiente de ejecución | Pendiente de ejecución |
| RF2 | PF-01 | PA-02 (PASS) | Pendiente de ejecución | Pendiente de ejecución |
| RF3 | PF-02, PF-15 | PA-02 (PASS) | Pendiente de ejecución | Pendiente de ejecución |
| RF4 | PF-03 | PA-03 (OK) | Pendiente de ejecución | Pendiente de ejecución |
| RF5 | PF-02 | PA-03 (OK) | Pendiente de ejecución | Pendiente de ejecución |
| RF6 | PF-03 | PA-03 (OK) | Pendiente de ejecución | Pendiente de ejecución |
| RF7 | PF-04 | PA-03 (OK) | Pendiente de ejecución | Pendiente de ejecución |
| RF8 | PF-04 | PA-03 (OK) | Pendiente de ejecución | Pendiente de ejecución |
| RF9 | PF-05 | PA-03 (OK) | Pendiente de ejecución | Pendiente de ejecución |
| RF10 | PF-06 | PA-03 (OK) | Pendiente de ejecución | Pendiente de ejecución |
| RF11 | PF-07, PF-08, PF-09 | PA-03 (OK) | Pendiente de ejecución | Pendiente de ejecución |
| RF12 | PF-10 | PA-03 (OK) | Pendiente de ejecución | Pendiente de ejecución |
| RF13 | PF-11 a PF-14 | PA-02 (PASS) | Pendiente de ejecución | Pendiente de ejecución |
| RF14 | PF-15, PF-16 | PA-03 (OK) | Pendiente de ejecución | Pendiente de ejecución |
| RF15 | PF-17 | — | Pendiente de ejecución | Pendiente de ejecución |

| RNF | Valor declarado | Valor medido | Estado |
|---|---|---|---|
| RNF1 Estabilidad | 15 min, 0 reinicios / 0 bloqueos / 0 reconexiones | Pendiente de ejecución | Pendiente de ejecución |
| RNF2 Exactitud HC-SR04 | ≤ 1 cm (5–50 cm) | Pendiente de ejecución | Pendiente de ejecución |
| RNF3 Exactitud IR | ≥ 95 % | Pendiente de ejecución | Pendiente de ejecución |
| RNF4 Respuesta por voz | ≤ 3,0 s | Pendiente de ejecución | Pendiente de ejecución |
| RNF5 Frecuencia de reporte | 30 s; muestras cada 300 s | Pendiente de ejecución | Pendiente de ejecución |
| RNF6 Reconexión | ≤ 60 s / ≤ 70 s (corte de 30 s) | Pendiente de ejecución | Pendiente de ejecución |
| RNF7 Integridad | ≥ 99 %; 4/4 inválidos | Pendiente de ejecución | Pendiente de ejecución |
| RNF8 Latencia de almacenamiento | Δ1 ≤ 5 s; Δ0 ≤ 15 s | Pendiente de ejecución | Pendiente de ejecución |
| RNF9 Lógica automática Edge | ≤ 0,3 s | Pendiente de ejecución | Pendiente de ejecución |
| RNF9 Lógica automática Nube | No aplica | No aplica | No aplica |
| RNF10 Operación sin conexión | E1–E4 operan; histórico encolado | Pendiente de ejecución | Pendiente de ejecución |
| RNF11 Costo | Dentro de la capa gratuita; < 0,30 USD/mes sin ella | Dentro de la capa gratuita en todos los conceptos excepto las escrituras bajo demanda de DynamoDB (≈ 0,01 USD/mes); sin capa gratuita ≈ 0,15 USD/mes (caso base) y ≈ 0,23 USD/mes (peor caso, Shadow por bloques de 1 KB), con tarifas verificadas de `us-east-1` (5.6) | Cumple por cálculo; se contrasta con la facturación de la sesión (8.4) |
| RNF12 Seguridad | Sin secretos; mínimo privilegio | Plantillas sin secretos y permisos de 3.2; PA-03 OK | Pendiente de ejecución (revisión final del repositorio) |

### 5.2 Resultados de las pruebas no funcionales

Tablas para completar con las hojas de registro.

**Tiempos.**

| Medición | n | Mínimo (s) | Promedio (s) | Máximo (s) | P95 (s) | Criterio | Estado |
|---|---|---|---|---|---|---|---|
| PNF-04 voz → fin de movimiento | 10 | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución | — (n < 20) | ≤ 3,0 s | Pendiente de ejecución |
| PNF-05 IR → LED / LCD | 10 | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución | — (n < 20) | ≤ 0,3 s | Pendiente de ejecución |
| PNF-06 Δ1 = REPORT − `receivedAt` | ≥ 20 | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución | ≤ 5 s | Pendiente de ejecución |
| PNF-06 Δ0 = `receivedAt` − `occurredAt` | ≥ 20 | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución | ≤ 15 s | Pendiente de ejecución |
| PNF-09 reconexión MQTT | 3 | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución | — | ≤ 60 s | Pendiente de ejecución |
| PNF-09 sincronización de `desired` | 3 | Pendiente de ejecución | Pendiente de ejecución | Pendiente de ejecución | — | ≤ 70 s | Pendiente de ejecución |

**Exactitud, estabilidad e integridad.**

| Medición | Resultado | Criterio | Estado |
|---|---|---|---|
| PNF-02 error máximo HC-SR04 | Pendiente de ejecución | ≤ 1 cm | Pendiente de ejecución |
| PNF-03 aciertos IR | Pendiente de ejecución | ≥ 95 % | Pendiente de ejecución |
| PNF-01 reinicios / bloqueos / reconexiones | Pendiente de ejecución | 0 / 0 / 0 | Pendiente de ejecución |
| PNF-07 M / N | Pendiente de ejecución | ≥ 99 % | Pendiente de ejecución |
| PNF-07 inválidos descartados | Pendiente de ejecución | 4 / 4 | Pendiente de ejecución |
| PNF-08 intervalo entre `update/accepted` | Pendiente de ejecución | 30 s ± 1 s | Pendiente de ejecución |
| PNF-10 ítems / transiciones durante el corte | Pendiente de ejecución | iguales | Pendiente de ejecución |

### 5.3 Datos almacenados en DynamoDB

**Muestra de ítems** (exportación CSV de la sesión en `docs/evidencias/pruebas/PF-07/`): Pendiente de ejecución.

**Consultas usadas** (consola de DynamoDB, *Query* sobre `SmartParkHistory`):

| Conteo | Partición / rango | Filtro |
|---|---|---|
| Por tipo de evento | `thingName = smartpark-grupo9-01`, `eventKey BETWEEN <inicio13># AND <fin13>#~` | `eventType = :tipo` (una vez por tipo) |
| Por origen (barrera) | ídem | `eventType = barrier_changed AND origin = :origen` |
| Por plaza | ídem | `eventType = occupancy_changed AND data.slot = :n` |

| Tipo de evento | Ítems | Origen (`barrier_changed`) | Ítems | Plaza (`occupancy_changed`) | Ítems |
|---|---|---|---|---|---|
| `sample` | Pendiente de ejecución | `legado` (voz V1) | Pendiente de ejecución | 1 | Pendiente de ejecución |
| `occupancy_changed` | Pendiente de ejecución | `edge` (Serial) | Pendiente de ejecución | 2 | Pendiente de ejecución |
| `barrier_changed` | Pendiente de ejecución | `voz` | Pendiente de ejecución | 3 | Pendiente de ejecución |
| Total | Pendiente de ejecución | `nube` | Pendiente de ejecución | 4 | Pendiente de ejecución |

### 5.4 Análisis de la decisión Edge / Nube

**Diseño.** Las cuatro reglas de control (2.6) se ejecutan en el Edge porque solo dependen del estado de las plazas y de la orden recibida, que el ESP32 conoce sin latencia de red. La alternativa en la Nube —una regla sobre `update/accepted` que invoque una Lambda que lea las plazas y rechace o reescriba `desired`— tendría tres desventajas medibles: (1) el tiempo de decisión pasaría de decenas de milisegundos a un viaje completo por AWS, (2) habría una ventana de carrera entre el último `reported` y la orden, en la que una apertura podría ejecutarse con el parqueo ya lleno, y (3) dejaría de funcionar sin conexión, justo cuando el encargado depende del objeto. Además añadiría una invocación de Lambda y mensajes por cada cambio de plaza.

La Nube aporta lo que el Edge no puede: **persistencia validada** del histórico, **consulta** de meses de datos para los reportes y **aislamiento por Thing** (la identidad sale del tópico autorizado). La separación también limita el daño de un error: la Lambda del histórico no tiene permiso para mover la barrera.

**Costo de la decisión.** La lógica en el Edge exige reprogramar el ESP32 para cambiar una regla; por eso los parámetros que conviene ajustar en operación (modo y antirrebote) se exponen por el Shadow y la voz (RF14). Las reglas que requieren histórico o varios objetos son candidatas naturales a la Nube (7.2).

**Confirmación con datos.** <!-- PENDIENTE: completar con resultados de 4.3 (PNF-05 frente a PNF-04 y PNF-06, y PNF-10) --> Tiempo de respuesta del Edge (PNF-05) frente al de un viaje por AWS (PNF-04 y Δ1 de PNF-06) y comportamiento sin conexión (PNF-10): Pendiente de ejecución.

### 5.5 Desviaciones y observaciones

| # | Observación | Efecto | Tratamiento |
|---|---|---|---|
| 1 | La skill escribe la barrera en formato V1 (`desired.barrera`) y el firmware registra esas órdenes con `origen`/`origin = legado`; el formato V2 con `origen = voz` existe en el firmware, pero ninguna Lambda lo emite. | En el histórico, las aperturas por voz aparecen como `legado` (M2). | Cambio recomendado en `ShadowService.set_barrier` (7.1). |
| 2 | Las órdenes rechazadas por E2 no se publican en el histórico. | M3 no puede contar aperturas rechazadas. | Evento `order_rejected` (7.1). |
| 3 | El firmware no imprime la causa del último reinicio ni la memoria libre. | Un reinicio en PNF-01 no se puede diagnosticar solo con el serie. | Cambio recomendado (7.1). |
| 4 | La prueba automática del modelo de Alexa valida `alexa/es-US.interaction-model.json`, no el modelo final `es-US.smartpark-v2.final.json`. | El modelo que se importa a la consola no tiene prueba automática. | Unificar el archivo (7.1). |
| 5 | El firmware no registra la hora de publicación de cada evento del histórico. | RNF8 se mide desde la llegada a AWS (`receivedAt`), no desde la publicación. | Cambio recomendado (7.1). |
| 6 | <!-- PENDIENTE: completar con resultados de 4.4 --> Desviaciones de la sesión de pruebas | Pendiente de ejecución | Pendiente de ejecución |

### 5.6 Costo mensual estimado

Escenario: un Thing, 30 días, **conexión continua** (cota superior; con el límite de demostración de 60 min por arranque el consumo real es mucho menor). Frecuencias del código: tope de 1000 solicitudes Shadow/día con su respuesta `accepted`; histórico de ≈ 570 eventos/día (288 muestras + 240 transiciones de plazas + 40 de barrera, 2.10); 50 interacciones de voz/día, 20 de ellas con escritura en `desired`. Lambda del histórico: 128 MB y 0,1 s supuestos; backend: 128 MB (configuración de [`lambda/README.md`](../lambda/README.md#L122)) y 1 s de duración supuesta.

| Servicio | Uso mensual | Capa gratuita | Costo sin capa gratuita (USD, tarifa `us-east-1` verificada) |
|---|---|---|---|
| IoT Core — mensajes | Shadow 30 000 + `accepted` 30 000 + histórico 17 100 + `delta` 600 ≈ 77 700 | 500 000/mes (12 meses) | 1,00/M → 0,078 |
| IoT Core — operaciones de Shadow | 30 000 (ESP32) + 1500 (Lambda) = 31 500 | 225 000/mes (registro + Shadow) | 1,25/M → 0,039 |
| IoT Core — conectividad | 43 200 min | 2 250 000 min/mes | 0,08/M min → 0,003 |
| IoT Core — reglas + acciones | 17 100 + 17 100 | 250 000 + 250 000/mes | 0,15/M + 0,15/M → 0,005 |
| Lambda — solicitudes | 17 100 + 1500 = 18 600 | 1 000 000/mes (siempre) | 0,20/M → 0,004 |
| Lambda — cómputo | 17 100 × 0,1 s × 0,125 GB + 1500 × 1 s × 0,125 GB ≈ 401 GB-s | 400 000 GB-s/mes (siempre) | 0,0000166667/GB-s → 0,007 |
| DynamoDB — escrituras bajo demanda | 17 100 WRU (ítems < 1 KB) | No cubiertas (la capa gratuita cubre capacidad aprovisionada) | 0,625/M → 0,011 |
| DynamoDB — lecturas | 1500 RRU (vinculación) + consultas de reportes | No cubiertas | 0,125/M → < 0,001 |
| DynamoDB — almacenamiento | ≈ 10 MB acumulados el primer mes (≈ 0,6 KB/ítem) | 25 GB (siempre) | < 0,01 |
| **Total** | | | **≈ 0,15 USD/mes** (peor caso ≈ 0,23 USD/mes, ver «Medición por tamaño»); con capa gratuita ≈ 0,01 USD/mes (escrituras bajo demanda) |

Cotas: aun en el máximo físico del histórico (8640 publicaciones/día = 259 200/mes) los mensajes siguen bajo 500 000, pero las reglas superarían por poco las 250 000 gratuitas. Las cuentas creadas desde el 15 de julio de 2025 reciben créditos de capa gratuita en lugar de la asignación de 12 meses de IoT Core.

**Verificación de tarifas.** Las tarifas por unidad se contrastaron el 2026-11-21 con la AWS Price List API para `us-east-1` (archivos publicados el 2026-10-03 para IoT Core, el 2026-11-20 para Lambda y el 2026-11-11 para DynamoDB) y con las páginas de precios (8.2). Coinciden todas: mensajes 1,00 USD/M (primeros 1000 M al mes), operaciones de Shadow y registro 1,25 USD/M, conectividad 0,08 USD/M min, reglas 0,15 USD/M y acciones 0,15 USD/M, Lambda 0,20 USD/M solicitudes y 0,0000166667 USD/GB-s (x86, primer tramo), DynamoDB bajo demanda 0,625 USD/M WRU y 0,125 USD/M RRU, y almacenamiento 0,25 USD/GB-mes más allá de los primeros 25 GB. No hubo que corregir ningún cálculo.

**Medición por tamaño.** IoT Core mide mensajes, reglas y acciones en incrementos de 5 KB, y el Shadow en incrementos de 1 KB del documento. Con los tamaños medidos en PA-02 (4.5), cada mensaje cuenta como uno y cada ítem del histórico como 1 WRU. El reporte del Shadow ocupa 476 B, pero el documento completo devuelto por GET ocupa 2458 B: si AWS factura cada actualización por el documento completo (3 operaciones), las operaciones de Shadow suben a 94 500/mes (0,118 USD) y el total sin capa gratuita a ≈ 0,23 USD/mes, aún dentro de las 225 000 operaciones gratuitas y bajo la cota de 0,30 USD/mes de RNF11 (1.4). La facturación de la sesión de pruebas (8.4) confirma cuál de los dos casos aplica.

---

## 6. Conclusiones

1. La ampliación responde a un caso de uso real y acotado: cuatro sensores IR dan la disponibilidad **por plaza**, que es la información que el encargado necesita, y el HC-SR04 se conserva solo para la pregunta distinta de si hay un vehículo en la barrera. Cada componente se usa en al menos una consulta, una regla o un reporte.
2. Colocar las cuatro reglas de control en el Edge es coherente con su naturaleza: dependen solo de datos locales, deben responder en decenas de milisegundos y deben seguir operando sin red. <!-- PENDIENTE: completar con resultados de 4.3 (PNF-05 y PNF-10) --> La verificación de ese tiempo y de la operación sin conexión depende de PNF-05 y PNF-10.
3. El Shadow y el tópico de histórico cumplen papeles distintos: el primero guarda el estado actual con cuota diaria y el segundo la secuencia de cambios con su propio presupuesto. Esa separación permite reportes exactos sin gastar la cuota de control.
4. La cadena de almacenamiento está diseñada para no aceptar datos falsos ni duplicados: la identidad sale del tópico autorizado, la Lambda valida tipos, rangos y coherencia, y la escritura condicional absorbe reintentos. <!-- PENDIENTE: completar con resultados de 4.3 (PNF-06 y PNF-07) --> La integridad y la latencia en AWS se confirman con PNF-06 y PNF-07.
5. La convivencia entre voz y lógica automática queda resuelta en un solo punto del firmware (`openingBlocked()`): la voz cambia el modo y pide movimientos, y el Edge decide. El costo es que Alexa no informa en la misma respuesta que una apertura fue bloqueada; el usuario lo confirma con una consulta.
6. El modelo de DynamoDB, con `thingName` y una clave de ordenamiento que empieza por la hora de ocurrencia, resuelve con una sola `Query` por rango todos los reportes diseñados para un Thing; solo un panel de varios Things o el conteo de aperturas rechazadas requieren cambios (7.1).
7. El costo estimado para un Thing, incluso con conexión continua, es del orden de céntimos de dólar al mes y queda dentro de la capa gratuita salvo las escrituras bajo demanda de DynamoDB.
8. La política del dispositivo, el permiso de invocación de la regla y los roles de las tres Lambdas están acotados a recursos concretos; el único comodín (`iot:DescribeEndpoint`) se puede eliminar configurando el endpoint.

---

## 7. Recomendaciones

### 7.1 Cambios técnicos sobre la implementación actual

- **Origen `voz` en el histórico:** que `ShadowService.set_barrier` escriba `desired.orden_barrera` (V2) con `id`, `origen: "voz"`, `solicitada_ms` y `vence_s`, ya soportado por el firmware; así M2 distingue voz de `legado` y el ESP32 identifica cada orden por su ID.
- **Evento `order_rejected`:** publicar en el histórico las órdenes bloqueadas o vencidas y aceptarlas en `validate_event()`, para completar M3.
- **TTL en `SmartParkHistory`:** atributo numérico `expiresAt` (epoch s) calculado en la Lambda, con retención de 13 meses para permitir comparaciones interanuales.
- **Índice para varios Things:** GSI con PK `eventType` y SK `eventKey` (proyección de `thingName`, `origin` y `data`) para paneles de varios parqueos sin `Scan`.
- **Diagnóstico de reinicios:** imprimir `esp_reset_reason()` al arrancar y `ESP.getFreeHeap()`/`ESP.getMinFreeHeap()` en `STATUS`.
- **Medición fina:** imprimir en el serie `epoch_ms` y `eventId` de cada publicación del histórico y registrar `eventKey` en el log de la Lambda, para medir RNF8 desde la publicación y emparejar sin ambigüedad.
- **Respuesta de voz más informativa:** tras escribir `desired`, que la consulta de estado lea `reported.ultima_orden` y explique un bloqueo.
- **Un solo modelo de Alexa:** que la prueba automática valide `es-US.smartpark-v2.final.json` y eliminar el archivo duplicado.
- **Reconexión más rápida:** limitar el *backoff* de Wi-Fi a 30 s mientras el corte sea corto y reiniciar el contador de intentos al reasociar, para no agotar los 10 intentos en cortes de ≈ 20 min.

### 7.2 Reglas de Nube para una versión real

| Regla | Implementación | Por qué en la Nube |
|---|---|---|
| Objeto sin reportar | Lambda programada con EventBridge cada 5 min: `Query` A6 por Thing; si el último `receivedAt` tiene > 15 min, registrar una alerta y notificar por SNS | El objeto no puede informar su propia desconexión |
| Límite diario de aperturas | Regla `SELECT … FROM 'smartpark/+/history/v1' WHERE eventType = 'barrier_changed'` → Lambda que cuenta las del día (`Query` A4) y, al superar el límite, escribe `desired.modo = "automatico"` | Necesita el histórico del día |
| Ocupación sostenida | Lambda programada que calcula la ocupación de la última hora y avisa al administrador si supera el 90 % | Agrega datos de varias horas o varios parqueos |

### 7.3 Seguridad y costo

- Configurar un presupuesto de AWS Budgets de 1 USD/mes con alerta por correo y revisar *Free Tier usage* en la consola.
- Eliminar `iot:DescribeEndpoint` fijando `IOT_DATA_ENDPOINT`.
- Usar variables de política (`${iot:Connection.Thing.ThingName}`) también en la política del Shadow, para que un único documento sirva a todos los Things.
- Agregar un destino de fallos asíncronos (cola SQS) a `SmartParkHistoryProcessor` para no perder eventos si DynamoDB falla tras los reintentos.

### 7.4 Extensiones hacia un uso real

- Sensores de plaza robustos a la luz ambiente (ultrasónicos o magnéticos de piso) y barrera con final de carrera para confirmar la posición.
- Persistir la cola del histórico en memoria flash y publicar con QoS 1 con acuse de la Lambda, para no perder eventos en reinicios.
- Panel web con los reportes M1–M4 sobre las consultas de 2.12 y exportación mensual a S3 para análisis.
- Varios Things por cuenta (un ESP32 por zona del parqueo) con agregación en la Nube.

---

## 8. Anexos

### 8.1 Glosario

| Término | Significado en este informe |
|---|---|
| Edge | Lógica que corre en el ESP32 |
| Shadow | Documento JSON de AWS IoT con `desired`, `reported` y `delta` del Thing |
| `delta` | Diferencia entre `desired` y `reported` que AWS envía al dispositivo |
| Antirrebote | Tiempo mínimo que una lectura del IR debe mantenerse para aceptarse |
| V1 / V2 | Formatos de orden de barrera: `desired.barrera` / `desired.orden_barrera` |
| `legado` | Origen asignado a una orden V1, que no declara su procedencia |
| WRU / RRU | Unidades de escritura y lectura bajo demanda de DynamoDB |
| P95 | Percentil 95 de una muestra |

### 8.2 Referencias

- Enunciado de la práctica: [`Práctica_4.md`](../Práctica_4.md).
- Repositorio de la Práctica 3: <https://github.com/HenryRiveraM/smartpark-practica-3>.
- Documentos técnicos de la V2: [`firmware-v2.md`](firmware-v2.md), [`historial-aws-practica-4.md`](historial-aws-practica-4.md), [`alexa-practica-4.md`](alexa-practica-4.md), [`lambda/README.md`](../lambda/README.md).
- AWS IoT Core, precios y capa gratuita (consultado el 2026-11-21): <https://aws.amazon.com/iot-core/pricing/>.
- AWS Lambda, precios (consultado el 2026-11-21): <https://aws.amazon.com/lambda/pricing/>.
- Amazon DynamoDB, precios bajo demanda (consultado el 2026-11-21): <https://aws.amazon.com/dynamodb/pricing/on-demand/>.
- AWS Price List API, ofertas vigentes para `us-east-1` (consultado el 2026-11-21): [AWS IoT](https://pricing.us-east-1.amazonaws.com/offers/v1.0/aws/AWSIoT/current/us-east-1/index.json), [AWS Lambda](https://pricing.us-east-1.amazonaws.com/offers/v1.0/aws/AWSLambda/current/us-east-1/index.json), [Amazon DynamoDB](https://pricing.us-east-1.amazonaws.com/offers/v1.0/aws/AmazonDynamoDB/current/us-east-1/index.json).
- Documento del Device Shadow: <https://docs.aws.amazon.com/iot/latest/developerguide/device-shadow-document.html>.
- Funciones SQL de AWS IoT: <https://docs.aws.amazon.com/iot/latest/developerguide/iot-sql-functions.html>.
- Variables de política del Thing: <https://docs.aws.amazon.com/iot/latest/developerguide/thing-policy-variables.html>.
- Escrituras condicionales de DynamoDB: <https://docs.aws.amazon.com/amazondynamodb/latest/developerguide/Expressions.ConditionExpressions.html>.
- Tipos de slot de Alexa: <https://developer.amazon.com/en-US/docs/alexa/custom-skills/slot-type-reference.html>.
- GPIO del ESP32: <https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html>.

### 8.3 Evidencias de las pruebas

| Evidencia | Ubicación | Estado |
|---|---|---|
| Hojas de registro | [`evidencias/hojas-registro/`](evidencias/hojas-registro/) | Preparadas |
| Salidas de las pruebas automáticas | [`pio-run.txt`](evidencias/pruebas-automaticas/pio-run.txt), [`host-tests.txt`](evidencias/pruebas-automaticas/host-tests.txt), [`lambda-unittest.txt`](evidencias/pruebas-automaticas/lambda-unittest.txt) | PA-01 SUCCESS; PA-02 5/5 PASS y contrato PASS (12 cargas); PA-03 102 OK |
| Evidencia por prueba (videos, capturas, CSV) | `docs/evidencias/pruebas/<ID>/` (una carpeta por PF-xx y PNF-xx) | Pendiente de ejecución |
| Muestra exportada de `SmartParkHistory` | `docs/evidencias/pruebas/PF-07/` | Pendiente de ejecución |

### 8.4 Evidencias de arquitectura y configuración

| Evidencia | Estado |
|---|---|
| Capturas de la consola: Thing, certificado activo y políticas adjuntas | Pendiente de ejecución |
| Regla `SmartParkHistoryIngest` habilitada y su acción | Pendiente de ejecución |
| Configuración de las tres Lambdas (runtime, memoria, *timeout*, variables sin valores secretos) | Pendiente de ejecución |
| Tablas `SmartParkBindings` y `SmartParkHistory` en estado `ACTIVE` | Pendiente de ejecución |
| Modelo de interacción construido y endpoint en la Alexa Developer Console | Pendiente de ejecución |
| Fotografías del montaje | Pendiente de ejecución |
| Facturación del mes de la sesión (Billing o Cost Explorer: mensajes, operaciones de Shadow, reglas y escrituras de DynamoDB), para contrastar 5.6 | Pendiente de ejecución |

### 8.5 Configuraciones exportadas

| Configuración | Archivo |
|---|---|
| Política IoT del dispositivo (Shadow) | [`docs/aws-iot-policy.example.json`](aws-iot-policy.example.json) |
| Política IoT complementaria (histórico) | [`aws/history/device-publish-policy.example.json`](../aws/history/device-publish-policy.example.json) |
| Regla de AWS IoT (SQL y acción) | [`aws/history/iot-rule.example.json`](../aws/history/iot-rule.example.json) |
| Permiso de invocación de la regla | [`aws/history/iot-invoke-permission.example.json`](../aws/history/iot-invoke-permission.example.json) |
| Rol del procesador del histórico | [`aws/history/processor-execution-policy.example.json`](../aws/history/processor-execution-policy.example.json) |
| Tabla `SmartParkHistory` | [`aws/history/table.example.json`](../aws/history/table.example.json) |
| Rol de `SmartParkAlexaBackend` | [`lambda/iam/alexa-execution-policy.example.json`](../lambda/iam/alexa-execution-policy.example.json) |
| Rol de `SmartParkPairingAdmin` y política del operador | [`lambda/iam/admin-execution-policy.example.json`](../lambda/iam/admin-execution-policy.example.json), [`lambda/iam/admin-invoke-policy.example.json`](../lambda/iam/admin-invoke-policy.example.json) |
| Confianza de los roles de Lambda | [`lambda/iam/execution-role-trust.example.json`](../lambda/iam/execution-role-trust.example.json) |
| Modelo de interacción de la skill | [`alexa/es-US.smartpark-v2.final.json`](../alexa/es-US.smartpark-v2.final.json) |
| Plantillas de configuración del objeto (sin secretos) | [`include/config.example.h`](../include/config.example.h), [`include/secrets.example.h`](../include/secrets.example.h) |
| Eventos de prueba de la skill | [`lambda/events/`](../lambda/events/) |

### 8.6 Diagramas

Los bloques Mermaid de la sección 2 se renderizan en GitHub. Como respaldo, cada uno está exportado a PNG con mermaid-cli:

| Diagrama | Fuente | Imagen |
|---|---|---|
| Bloques (2.1) | [`01-bloques.mmd`](diagrams/01-bloques.mmd) | [`01-bloques.png`](diagrams/01-bloques.png) |
| Arquitectura (2.2) | [`02-arquitectura.html`](diagrams/02-arquitectura.html) | [`02-arquitectura.png`](diagrams/02-arquitectura.png) |
| Circuito (2.3) | [`03-circuito.excalidraw`](diagrams/03-circuito.excalidraw) | [`03-circuito.png`](diagrams/03-circuito.png), [`03-circuito.svg`](diagrams/03-circuito.svg) |
| Clases del firmware (2.4.1) | [`04-clases-firmware.mmd`](diagrams/04-clases-firmware.mmd) | [`04-clases-firmware.png`](diagrams/04-clases-firmware.png) |
| Clases de las Lambdas (2.4.2) | [`05-clases-lambdas.mmd`](diagrams/05-clases-lambdas.mmd) | [`05-clases-lambdas.png`](diagrams/05-clases-lambdas.png) |
| Secuencia de consulta (2.5.1) | [`06-secuencia-consulta.mmd`](diagrams/06-secuencia-consulta.mmd) | [`06-secuencia-consulta.png`](diagrams/06-secuencia-consulta.png) |
| Secuencia de comando (2.5.1) | [`07-secuencia-comando.mmd`](diagrams/07-secuencia-comando.mmd) | [`07-secuencia-comando.png`](diagrams/07-secuencia-comando.png) |
| Secuencia de cambio de parámetro (2.5.1) | [`07b-secuencia-modo.mmd`](diagrams/07b-secuencia-modo.mmd) | [`07b-secuencia-modo.png`](diagrams/07b-secuencia-modo.png) |
| Secuencia de almacenamiento (2.5.2) | [`08-secuencia-almacenamiento.mmd`](diagrams/08-secuencia-almacenamiento.mmd) | [`08-secuencia-almacenamiento.png`](diagrams/08-secuencia-almacenamiento.png) |
| Regla E1 (2.5.3) | [`09-secuencia-edge-plaza.mmd`](diagrams/09-secuencia-edge-plaza.mmd) | [`09-secuencia-edge-plaza.png`](diagrams/09-secuencia-edge-plaza.png) |
| Reglas E2–E4 (2.5.3) | [`10-secuencia-edge-bloqueo.mmd`](diagrams/10-secuencia-edge-bloqueo.mmd) | [`10-secuencia-edge-bloqueo.png`](diagrams/10-secuencia-edge-bloqueo.png) |
| Estados del modo (2.6) | [`11-estados-control.mmd`](diagrams/11-estados-control.mmd) | [`11-estados-control.png`](diagrams/11-estados-control.png) |
| Estados de la barrera (2.6) | [`11b-estados-barrera.mmd`](diagrams/11b-estados-barrera.mmd) | [`11b-estados-barrera.png`](diagrams/11b-estados-barrera.png) |
| Estados de una plaza (2.6) | [`12-estados-plaza.mmd`](diagrams/12-estados-plaza.mmd) | [`12-estados-plaza.png`](diagrams/12-estados-plaza.png) |
| Modelo de datos (2.8) | [`13-modelo-datos.mmd`](diagrams/13-modelo-datos.mmd) | [`13-modelo-datos.png`](diagrams/13-modelo-datos.png) |

### 8.7 Mockups

| Reporte | Fuente | Imagen |
|---|---|---|
| M1 Ocupación por plaza y franja horaria | [`M1-ocupacion-por-plaza.excalidraw`](mockups/M1-ocupacion-por-plaza.excalidraw) | [`PNG`](mockups/M1-ocupacion-por-plaza.png), [`SVG`](mockups/M1-ocupacion-por-plaza.svg) |
| M2 Aperturas de barrera por origen | [`M2-aperturas-por-origen.excalidraw`](mockups/M2-aperturas-por-origen.excalidraw) | [`PNG`](mockups/M2-aperturas-por-origen.png), [`SVG`](mockups/M2-aperturas-por-origen.svg) |
| M3 Saturación | [`M3-saturacion.excalidraw`](mockups/M3-saturacion.excalidraw) | [`PNG`](mockups/M3-saturacion.png), [`SVG`](mockups/M3-saturacion.svg) |
| M4 Permanencia por plaza | [`M4-permanencia.excalidraw`](mockups/M4-permanencia.excalidraw) | [`PNG`](mockups/M4-permanencia.png), [`SVG`](mockups/M4-permanencia.svg) |
