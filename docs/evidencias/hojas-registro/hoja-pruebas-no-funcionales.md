# Hoja de registro — pruebas no funcionales (Práctica 4)

Imprimir antes de medir. Valores declarados y fórmulas: [informe, secciones 1.4 y 4.3](../../informe-practica-4.md#43-pruebas-no-funcionales). Todos los tiempos en segundos con décimas (o milisegundos cuando la fuente los da).

**Orígenes de tiempo**

| Código | Fuente | Resolución | Uso |
|---|---|---|---|
| S | Monitor serie con marca del PC (`pio device monitor -f time`), PC sincronizado por NTP | 1 ms (marca de llegada al PC) | eventos del firmware |
| V | Video del celular a ≥ 30 fps, contado por cuadros | 33 ms a 30 fps | voz → servo, IR → LED |
| R | `receivedAt` del ítem (función `timestamp()` de la regla de AWS IoT) | 1 ms | llegada a AWS IoT Core |
| O | `occurredAt` del ítem (reloj del ESP32, NTP) | 1 ms | ocurrencia en el objeto |
| C | Hora del registro `REPORT` / `END` en CloudWatch Logs (UTC) | 1 ms | fin de la ejecución de la Lambda |

- Fecha: ____________  Responsable: ____________________  fps del video: ______  Desfase PC − hora NTP (s): ______

## PNF-01 Estabilidad (≥ 15 min conectado, uso normal)

| Inicio (S) | Fin (S) | Duración (min) | Reinicios (banner `=== SmartPark V2`) | Desconexiones (`reconexiones=`) | Bloqueos (sin salida serie > 5 s) | `esp_reset_reason` / observación |
|---|---|---|---|---|---|---|
|  |  |  |  |  |  |  |

## PNF-02 Exactitud del HC-SR04 (5 lecturas por distancia, comando `DISTANCE`)

| Distancia real (cm) | L1 | L2 | L3 | L4 | L5 | Error abs. máx. (cm) |
|---|---|---|---|---|---|---|
| 5 |  |  |  |  |  |  |
| 10 |  |  |  |  |  |  |
| 15 |  |  |  |  |  |  |
| 20 |  |  |  |  |  |  |
| 30 |  |  |  |  |  |  |
| 50 |  |  |  |  |  |  |

## PNF-03 Exactitud de los IR (10 entradas y 10 salidas por plaza)

| Plaza | Entradas correctas / 10 | Salidas correctas / 10 | Estados `desconocido` observados | Observación |
|---|---|---|---|---|
| 1 |  |  |  |  |
| 2 |  |  |  |  |
| 3 |  |  |  |  |
| 4 |  |  |  |  |

## PNF-04 Tiempo de respuesta por voz (fin de la frase «abre la barrera» / «cierra la barrera» → fin del movimiento)

| Rep. | Frase | Cuadro fin de frase | Cuadro fin de movimiento | Δ cuadros | Δt (s) = Δ cuadros / fps | Observación |
|---|---|---|---|---|---|---|
| 1 | abre la barrera |  |  |  |  |  |
| 2 | cierra la barrera |  |  |  |  |  |
| 3 | abre la barrera |  |  |  |  |  |
| 4 | cierra la barrera |  |  |  |  |  |
| 5 | abre la barrera |  |  |  |  |  |
| 6 | cierra la barrera |  |  |  |  |  |
| 7 | abre la barrera |  |  |  |  |  |
| 8 | cierra la barrera |  |  |  |  |  |
| 9 | abre la barrera |  |  |  |  |  |
| 10 | cierra la barrera |  |  |  |  |  |

## PNF-05 Respuesta de la lógica Edge (IR cubierto → LED cambia; plaza 4 cubierta → LCD `APERTURA BLOQUEADA`)

| Rep. | Regla | Cuadro condición | Cuadro cambio del actuador/indicador | Δt (s) | Antirrebote vigente (ms) | Observación |
|---|---|---|---|---|---|---|
| 1 | E1 LED |  |  |  |  |  |
| 2 | E1 LED |  |  |  |  |  |
| 3 | E1 LED |  |  |  |  |  |
| 4 | E1 LED |  |  |  |  |  |
| 5 | E1 LED |  |  |  |  |  |
| 6 | E1 LED |  |  |  |  |  |
| 7 | E1 LED |  |  |  |  |  |
| 8 | E2 LCD |  |  |  |  |  |
| 9 | E2 LCD |  |  |  |  |  |
| 10 | E2 LCD |  |  |  |  |  |

## PNF-06 Latencia de almacenamiento (≥ 20 eventos)

| # | eventKey | occurredAt O (ms) | receivedAt R (ms) | REPORT C (hh:mm:ss.mmm) | Δ0 = R − O (s) | Δ1 = C − R (s) | Observación |
|---|---|---|---|---|---|---|---|
| 1 |  |  |  |  |  |  |  |
| 2 |  |  |  |  |  |  |  |
| 3 |  |  |  |  |  |  |  |
| 4 |  |  |  |  |  |  |  |
| 5 |  |  |  |  |  |  |  |
| 6 |  |  |  |  |  |  |  |
| 7 |  |  |  |  |  |  |  |
| 8 |  |  |  |  |  |  |  |
| 9 |  |  |  |  |  |  |  |
| 10 |  |  |  |  |  |  |  |
| 11 |  |  |  |  |  |  |  |
| 12 |  |  |  |  |  |  |  |
| 13 |  |  |  |  |  |  |  |
| 14 |  |  |  |  |  |  |  |
| 15 |  |  |  |  |  |  |  |
| 16 |  |  |  |  |  |  |  |
| 17 |  |  |  |  |  |  |  |
| 18 |  |  |  |  |  |  |  |
| 19 |  |  |  |  |  |  |  |
| 20 |  |  |  |  |  |  |  |

## PNF-07 Integridad del almacenamiento (ventana de 10 min)

| Inicio ventana (S) | Fin ventana (S) | `Historico: MQTT local` al inicio | al fin | Publicados N | Ítems en DynamoDB con `receivedAt` en la ventana M | M / N (%) | `cola_llena` | `fallos_envio` | `descartes_red` |
|---|---|---|---|---|---|---|---|---|---|
|  |  |  |  |  |  |  |  |  |  |

| Inválido # | JSON publicado desde el cliente MQTT (resumen) | Línea `history_invalid field=` en CloudWatch | ¿Ítem creado? (debe ser No) |
|---|---|---|---|
| 1 | `distancia_cm: 999` |  |  |
| 2 | `capacidad: 5` |  |  |
| 3 | `eventType: "door_opened"` |  |  |
| 4 | `origin: "voz"` en `sample` |  |  |
| 5 | duplicado de un evento válido (mismo `eventId` y `occurredAt`) | (no aplica: se espera 1 solo ítem) |  |

## PNF-08 Frecuencia de reporte (5 min en reposo, cliente MQTT suscrito a `$aws/things/smartpark-grupo9-01/shadow/update/accepted`)

| Inicio (S) | Fin (S) | `update/accepted` recibidos | Intervalo medio (s) | `solicitudesTotales` inicio / fin (`STATUS`) | Muestras `sample` en DynamoDB en 15 min |
|---|---|---|---|---|---|
|  |  |  |  |  |  |

## PNF-09 Reconexión y sincronización (corte de 30 s, 3 repeticiones)

| Rep. | AP apagado (S) | AP encendido (S) | Duración corte (s) | `desired.modo` escrito durante el corte (hora) | `AWS MQTT/TLS conectado` (S) | `reported.modo` actualizado (hora del `update/accepted`) | t reconexión = conectado − encendido (s) | t sincronización (s) |
|---|---|---|---|---|---|---|---|---|
| 1 |  |  |  |  |  |  |  |  |
| 2 |  |  |  |  |  |  |  |  |
| 3 |  |  |  |  |  |  |  |  |

## PNF-10 Operación sin conexión (corte de 2 min)

| Acción durante el corte | ¿Funciona? (S/N) | Observación |
|---|---|---|
| Cubrir y liberar plaza 1 (LED y LCD) |  |  |
| Modo automático con 4 plazas ocupadas + `OPEN` por Serial (debe bloquear) |  |  |
| `CLOSE` por Serial |  |  |
| Transiciones generadas durante el corte (contar) |  |  |
| Ítems de esas transiciones en DynamoDB tras reconectar (contar) |  |  |
| Tiempo de drenado de la cola tras reconectar (s) |  |  |
