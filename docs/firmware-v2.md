# Firmware SmartPark V2

## Cableado del montaje

| Señal | GPIO |
|---|---:|
| HC-SR04 TRIG / ECHO | 32 / 35 |
| SG90 PWM | 18 |
| LCD 16×2 I²C SDA / SCL | 21 / 22 |
| Slot 1 IR / rojo / verde | 34 / 17 / 13 |
| Slot 2 IR / rojo / verde | 36 / 14 / 19 |
| Slot 3 IR / rojo / verde | 39 / 23 / 25 |
| Slot 4 IR / rojo / verde | 33 / 26 / 27 |

El mapa aplica al objetivo PlatformIO `esp32doit-devkit-v1`. Antes de cablear, comprobar el número de parte del módulo, que GPIO 17 esté libre de PSRAM y que los GPIO 36 y 39 estén expuestos (pueden figurar como VP y VN). Los GPIO 34, 35, 36 y 39 solo admiten entrada y carecen de resistencias internas de polarización. Los IR deben entregar niveles digitales válidos de 3,3 V. Para una salida de colector abierto, usar polarización externa a 3,3 V. Si el módulo solo funciona a 5 V y su salida alcanza esa tensión, adaptar el nivel antes del GPIO. Verificar que el módulo es activo bajo o ajustar `IR_OCCUPIED_LEVEL`. [GPIO del ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)

El firmware usa un backpack PCF8574 con dirección inicial `0x27`, datos en P4–P7 y RS/RW/EN en P0/P1/P2. El adaptador controla solo la LCD. Confirmar su dirección, polaridad de luz y correspondencia de pines. Cada LED necesita su propia resistencia. Para un objetivo inicial de 2 mA, calcularla con `R = (3,3 V − Vf) / 0,002 A` y elegir el valor comercial superior. En el montaje hay 4 resistencias de 220 Ω y 4 de 330 Ω, una en serie con cada LED; no está registrado qué valor lleva cada color. [Ficha PCF8574](https://www.ti.com/product/PCF8574)

**Montaje real.** Todo se alimenta por el USB del ESP32. El HC-SR04 y la LCD (backpack a 5 V) toman 5 V del pin VIN; los cuatro IR y el servo SG90, 3,3 V del pin 3V3. ECHO llega directo a GPIO 35 y SDA/SCL directos a GPIO 21/22. Todas las masas están unidas al GND del ESP32. Diagrama: [`03-circuito.png`](diagrams/03-circuito.png).

**Recomendaciones para una versión real.** Divisor de voltaje en ECHO, porque GPIO 35 trabaja a 3,3 V y el HC-SR04 a 5 V entrega pulsos de 5 V. Traductor de nivel bidireccional I²C para el backpack a 5 V, con pull-ups del lado de cada tensión. SG90 a 5 V con una fuente aparte y GND común: su rango nominal es 4,8–6 V y a 3,3 V tiene menos torque y puede tardar más que `SERVO_TRAVEL_MS`.

## LCD 16×2

Si la pantalla muestra caracteres corruptos y el monitor serie sigue respondiendo, envía `LCDRESET` a 115200 baudios con terminación de línea. El comando solicita la reinicialización de la LCD y el redibujo de ambas filas desde el bucle normal. La confirmación de Serial indica que se recibió la orden. Usa `SLOT1` para consultar el estado I²C y comprueba visualmente que vuelve el texto. El comando no acciona el servo ni reinicia el ESP32. La inicialización espera 100 ms, fija E, RS y R/W en bajo (`prepareExpander()`), envía 3-3-3-2, `0x28 0x0C 0x01` y luego `0x06`, con 5 ms entre pasos.

Para descartar el cableado o el módulo, `pio run -e lcd-diagnostico -t upload` carga [`src/lcd_diagnostico.cpp`](../src/lcd_diagnostico.cpp) en lugar del firmware: sondea la dirección 0x27, inicializa la pantalla con la biblioteca hd44780 y relee la DDRAM de cada fila (`Fila n: lectura de memoria OK`). Después se vuelve a cargar el firmware principal con `pio run -t upload`.

`ParkingDisplay` controla una LCD 16×2 (HD44780 en modo de 4 bits por el PCF8574; filas en las direcciones DDRAM 0x00 y 0x40). Escribe una fila por llamada y solo si su texto cambió, sin bloquear `loop()`. Ante un error I²C reinicia la inicialización y reescribe ambas filas. El tamaño está en `LCD_COLUMNS = 16` y `LCD_ROWS = 2`, con valor por defecto en [`ParkingDisplay.h`](../include/ParkingDisplay.h).

| Fila | Contenido |
|---|---|
| 0 | Plazas libres/capacidad y ocupadas: `Libres:3/4 Oc:1`. |
| 1 | Por prioridad: `APERTURA BLOQ.` si la apertura está bloqueada (modo automático sin plaza confirmada libre); si no, `PARQUEO LLENO` si las cuatro plazas están ocupadas; si no, alterna cada 2 s entre el modo (`Modo: manual` / `Modo: automatico`) y la barrera (`Barrera: abierta`, `Barrera: cerrada`, `Barrera: mueve` durante el recorrido o `Barrera: ---` sin posición estimada). |

## Diagnóstico físico de una plaza

Para un LED discreto con resistencia hacia GND, evita GPIO 15 como primera opción. En el ESP32 clásico, GPIO 15 (MTDO) se muestrea al reset y tiene pull-up interno. Una carga que lleve el pin a nivel bajo puede silenciar los mensajes de arranque por UART0 y cambiar la temporización del esclavo SDIO. GPIO 4 aparece en el cabezal de la DOIT DevKit V1 de 30 pines, no es de strapping y está libre en el mapa de SmartPark. Confirma la marca `D4` o `IO4` de tu revisión. GPIO 17 suele figurar como `TX2`. [Strapping del ESP32](https://documentation.espressif.com/projects/arduino-esp32/en/latest/boards/ESP32-DevKitC-1.html) · [Esquema DOIT DevKit V1](https://mischianti.org/wp-content/uploads/2024/11/DOIT-ESP32-DevKit-V1-schematics.pdf)

1. Para el ensayo con GPIO 4, en tu `include/config.h` local establece `PARKING_CAPACITY = 1` y cambia solo la primera entrada de `SLOT_PINS` a `{34, 4, 13}`. Con la placa apagada, pasa el LED rojo a D4 y usa una resistencia en serie. Mantén las señales del IR compatibles con 3,3 V.
2. Carga el firmware y abre el monitor serie a 115200, con terminación de línea. Envía `SLOT1` con el sensor sin obstáculo y luego con un obstáculo. Con el IR activo bajo, tras estabilizarse la lectura se espera `1 / LIBRE / rojo OFF / verde ON` y `0 / OCUPADO / rojo ON / verde OFF`, respectivamente. Retira el obstáculo y comprueba que vuelve a libre. El comando muestra la lectura cruda y el estado filtrado, que pueden diferir durante el antirrebote.
3. Confirma visualmente los LEDs y que la fila 0 de la LCD pasa entre `Libres:1/1 Oc:0` y `Libres:0/1 Oc:1`. El diagnóstico muestra si la dirección I²C configurada responde y si el controlador completó la inicialización. La respuesta I²C por sí sola no verifica el texto ni el contraste. Si usas Wi-Fi durante el ensayo, la configuración normal del firmware podría reportar el estado de una plaza al Shadow.

## Presupuesto de corriente y pruebas de banco

La opción de repartir 5 V y 3,3 V desde el DevKit depende de la capacidad del USB, el cable, la ruta real hasta VIN/5 V, el regulador y la protoboard. Comprobar primero si VIN está conectado al USB en la revisión concreta y la corriente admisible de cada tramo. El presupuesto siguiente solo sirve para dimensionar el ensayo.

| Carga | Referencia o supuesto inicial |
|---|---|
| ESP32 | 240 mA típicos del chip en la condición de transmisión 802.11b de la ficha. Espressif recomienda una alimentación de 3,3 V capaz de entregar al menos 500 mA para el diseño del ESP32. |
| Indicadores | ≈8 mA con cuatro LEDs activos a 2 mA por LED. |
| Cuatro IR | Reservar provisionalmente 20 mA por módulo (80 mA). Medir a 3,3 V con el emisor activo y en ambos estados. |
| HC-SR04 y LCD | Reservar provisionalmente 20 mA para el ultrasónico y 40 mA para LCD con luz. Medir ambos módulos. |
| SG90 | Presupuestar inicialmente 0,7 A de pico como hipótesis de ensayo. Medir arranque, giro y carga con la unidad real. La ficha del SG90 analógico especifica 4,8 V de operación. |

Con esos supuestos, los consumos suman aproximadamente **0,39 A sin servo y 1,09 A con el pico supuesto**, antes de añadir pérdidas y margen. El estimado descarta dar por hecho que un puerto o la ruta VIN de la placa puedan sostener el conjunto. Si la ruta USB/VIN es insuficiente, alimentar el servo con una fuente regulada independiente de 5 V, dimensionada a partir de la corriente medida y con masa común. Mantener separados los positivos del USB y de la fuente externa. [Ficha del ESP32](https://documentation.espressif.com/esp32_datasheet_en.html) · [Guía de alimentación del ESP32](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/schematic-checklist.html) · [SG90 analógico](https://towerpro.com.tw/product/sg90-analog/)

1. Con la placa desenergizada, verificar continuidad de masas, polaridad, límites del regulador, rieles de la protoboard y ausencia de cortos. Confirmar las etiquetas y variante del módulo.
2. Alimentar por etapas. Medir ECHO, IR y ambos lados de I²C antes de conectar las señales al ESP32. Verificar estado inactivo y activo de cada sensor.
3. Medir corriente y tensión en los rieles con Wi‑Fi transmitiendo, LCD con luz e IR activos. Observar transitorios de 5 V y 3,3 V con osciloscopio durante órdenes del servo con y sin carga normal. Vigilar reinicios, pérdida de I²C y calentamiento. Si aparecen caídas, reinicios o se supera un límite documentado, usar la alimentación independiente y repetir.
4. Comprobar arranque en frío, transiciones de cada slot, luz ambiente, umbral del IR, latencia de LEDs/LCD, pérdida de Wi‑Fi y ausencia de movimiento al arranque o reconectar. No bloquear mecánicamente la barrera para provocar una sobrecarga.

## Control local y prioridad

El firmware muestrea cada IR nominalmente cada 10 ms y exige 120 ms de estabilidad de fábrica. El antirrebote admite enteros de 50 a 1.000 ms. Cada slot arranca en `desconocido`, con ambos LEDs apagados. Un estado estable activa solo el LED correspondiente. Una lectura digital inválida queda como desconocida. Un cable desconectado que entregue un nivel válido y constante no se reconoce como fallo por software.

`lleno` significa cuatro plazas confirmadas ocupadas. En automático se bloquea la apertura cuando no existe ninguna plaza confirmada libre, incluidos arranque y lecturas desconocidas. Manual permite órdenes explícitas de apertura y cierre, y automático siempre admite el cierre explícito. Las lecturas, los cambios de modo, la liberación de plazas y el arranque no emiten órdenes PWM. Hasta la primera orden explícita no se inicializa el PWM y la posición de barrera queda sin estimar. El SG90 no tiene realimentación y su posición tras una orden es solo una estimación temporal.

En cada vuelta, una configuración remota válida se aplica antes de decidir un movimiento. La petición Serial más reciente de esa vuelta prevalece sobre una petición remota pendiente. Entre órdenes remotas pendientes, prevalece la más reciente aceptada. La pérdida de conexión invalida movimientos en cola. Las opciones de modo y antirrebote se guardan en NVS antes de aplicarse y permanecen tras reiniciar. Los comandos Serial son `OPEN`, `CLOSE`, `STATUS`, `DISTANCE`, `SLOT1`, `LCDRESET` y `HELP`; si llegan varias órdenes de barrera en la misma vuelta, se procesa la última.

## Device Shadow clásico

Ejemplo de actualización emitida por el ESP32. Los valores ilustran el formato.

```json
{
  "clientToken": "12345678-41",
  "state": {
    "reported": {
      "barrera": "cerrada",
      "barrera_posicion_estimada": true,
      "vehiculo_detectado": true,
      "distancia_cm": 12.4,
      "slots": {"1":"ocupado","2":"ocupado","3":"ocupado","4":"ocupado"},
      "ocupados": 4,
      "disponibles": 0,
      "capacidad": 4,
      "modo": "automatico",
      "lleno": true,
      "bloqueo_apertura": true,
      "parametros_auto": {"antirrebote_ir_ms":120},
      "ultima_orden": {
        "id": null,
        "objetivo": "abierta",
        "origen": "legado",
        "resultado": "bloqueada"
      }
    }
  }
}
```

`barrera` se envía como `null` cuando todavía no hay posición estimada. `distancia_cm` y `vehiculo_detectado` salen de la misma muestra del HC-SR04 (presente ≤ 15 cm, ausente ≥ 18 cm); sin lectura válida ambos se envían como `null` y AWS elimina las claves. Las 3 muestras estables (`AWS_PRESENCE_STABLE_SAMPLES`) solo deciden si un cambio de presencia adelanta un reporte. El ejemplo muestra una orden V1 de la skill, que no lleva ID y se registra con origen `legado`. `ultima_orden` puede faltar antes de procesar una orden. Sus orígenes son `voz`, `nube`, `edge` y `legado`, y sus resultados `aceptada`, `bloqueada`, `vencida` o `no_disponible`. `aceptada` indica que el controlador aceptó el objetivo, incluso si la barrera ya estaba en esa posición estimada.

Ejemplo del formato V2 de escritura. El productor debe regenerar los tiempos para cada orden nueva. Los reintentos conservan tanto ID como tiempos.

```json
{
  "state": {
    "desired": {
      "modo": "automatico",
      "parametros_auto": {"antirrebote_ir_ms":120},
      "orden_barrera": {
        "id": "solicitud-42",
        "objetivo": "abierta",
        "origen": "voz",
        "solicitada_ms": 1795000000000,
        "vence_s": 1795000030
      }
    }
  }
}
```

El ID admite 1–32 caracteres alfanuméricos, guion y guion bajo. `solicitada_ms` es Unix UTC en milisegundos y aumenta entre órdenes V2 del mismo Thing. `vence_s` es Unix UTC en segundos y limita la vigencia a un máximo de 30 s. La versión del Shadow ordena GET y deltas; el token correlaciona los acuses. El firmware registra una orden en NVS antes de evaluarla para el servo, incluso si se bloquea o se pierde en una reconexión. Una orden consumida requiere una solicitud nueva para intentar mover la barrera otra vez. El firmware no borra el `desired` de otro cliente.

El cliente V1 sigue escribiendo `{"state":{"desired":{"barrera":"abierta"}}}`. El firmware exige metadatos de AWS con hora válida y reciente. Para ese formato, el par objetivo/timestamp de AWS identifica la orden; las peticiones repetidas del mismo objetivo en el mismo segundo son indistinguibles. Si ambos formatos coexisten, el firmware usa los metadatos de AWS para elegir al más reciente y da prioridad a V2 si empatan. [Formato y metadatos del Device Shadow](https://docs.aws.amazon.com/iot/latest/developerguide/device-shadow-document.html)

## Verificación local

La prueba de tamaño serializa el reporte con el código del firmware y construye respuestas representativas con metadatos AWS y ambos formatos `desired`. Midió **477 B** para el reporte, **2.459 B** para GET, **672 B** para delta y **1.888 B** para UPDATE accepted ([`host-tests.txt`](evidencias/pruebas-automaticas/host-tests.txt)). El GET ocupó hasta **2.519 B** en el cálculo conservador para el tópico configurado y encabezado MQTT. Se aumentó el búfer de 1.024 a 3.072 B. Los límites heredados de publicación siguen vigentes. Los cambios de ocupación comparten el presupuesto de sensores, y las respuestas de control comparten la reserva de control. Las respuestas se simularon localmente, sin consultar AWS.

`pio run` compila para el DevKit. Con el Shadow, el monitor serie imprime al arrancar la marca `AWS Shadow diagnostico v1 compilado=…`, en cada UPDATE la línea `AWS Shadow TX` (bytes, escritura local y JSON) y en cada acuse `AWS Shadow RX update/accepted|rejected` con `coincide` y `version`. `sh test/run_host_tests.sh` usa dobles para GPIO, I²C, servo, NVS, Wi‑Fi y MQTT, y ArduinoJson real. Las comprobaciones de alimentación, módulos físicos, latencia real, recorrido del servo y sesión con AWS siguen pendientes.

El publicador de histórico comparte el cliente y la tarea MQTT. Consulta el [contrato, las garantías de entrega y el orden de despliegue](historial-aws-practica-4.md#publicador-integrado-en-el-firmware).
