# Hoja de registro — pruebas no funcionales (Práctica 4)

Imprimir antes de medir. Valores declarados, criterios y fórmulas: [informe, secciones 1.4 y 4.3](../../informe-practica-4.md#43-pruebas-no-funcionales). Tiempos en segundos con décimas, o en milisegundos cuando la fuente los da.

**Orígenes de tiempo**

| Código | Fuente | Resolución | Uso |
|---|---|---|---|
| S | Log serie con la hora del PC (`pio device monitor -f time`), PC sincronizado por NTP | 1 ms (llegada al PC) | `STATUS`, conexión, `AWS UPDATE` |
| V | Video del celular a ≥ 30 fps, contado por cuadros | 33 ms a 30 fps | voz → servo, IR → LED |
| R | `receivedAt` del ítem (`timestamp()` de la regla de AWS IoT) | 1 ms | llegada a AWS IoT Core |
| O | `occurredAt` del ítem (reloj NTP del ESP32) | 1 ms | ocurrencia en el objeto |
| C | `@timestamp` del registro `REPORT` en CloudWatch Logs (UTC) | 1 ms | fin de la ejecución de la Lambda |

- Fecha: ____________  Responsables: ______________________________  fps del video: ______

## PNF-01 Estabilidad (RNF1, toda la sesión)

| `STATUS` inicial (S) | `STATUS` final (S) | Duración (min) | Reinicios (banner `=== SmartPark V2`) | Reconexiones no provocadas (fuera de PNF-06) | Bloqueos (`STATUS` sin respuesta) |
|---|---|---|---|---|---|
|  |  |  |  |  |  |

## PNF-02 Exactitud (RNF2 y RNF3)

**HC-SR04** (bloque 2; 5 lecturas `DISTANCE` por distancia)

| Distancia real (cm) | L1 | L2 | L3 | L4 | L5 | Error abs. máx. (cm) |
|---|---|---|---|---|---|---|
| 10 |  |  |  |  |  |  |
| 20 |  |  |  |  |  |  |
| 30 |  |  |  |  |  |  |

**IR** (dentro de la ventana de PNF-05; una transición cada 15 s; marcar ✓ si el LED y el estado coinciden con la realidad, ✗ si no)

| Plaza | E1 | S1 | E2 | S2 | E3 | S3 | E4 | S4 | E5 | S5 | Aciertos / 10 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 |  |  |  |  |  |  |  |  |  |  |  |
| 2 |  |  |  |  |  |  |  |  |  |  |  |
| 3 |  |  |  |  |  |  |  |  |  |  |  |
| 4 |  |  |  |  |  |  |  |  |  |  |  |

Total de aciertos: ______ / 40  (criterio ≥ 38)

## PNF-03 Respuesta por voz (RNF4, video de PF-02)

| Rep. | Orden | Cuadro de la frase enviada | Cuadro en que la barrera se detiene | Δcuadros | t (s) |
|---|---|---|---|---|---|
| 1 | abre |  |  |  |  |
| 2 | cierra |  |  |  |  |
| 3 | abre |  |  |  |  |
| 4 | cierra |  |  |  |  |
| 5 | abre |  |  |  |  |
| 6 | cierra |  |  |  |  |

## PNF-04 Lógica automática en el Edge (RNF9, video de PF-06)

LCD esperada en el video: la fila 0 sigue cada ocupación (`Libres:3/4 Oc:1` … `Libres:0/4 Oc:4`); con la 4.ª ocupación en `automatico`, la fila 1 muestra `APERTURA BLOQ.`.

| Rep. | Plaza | Cuadro en que el vehículo cubre el IR | Cuadro del cambio del LED | Δcuadros | t (s) |
|---|---|---|---|---|---|
| 1 | 1 |  |  |  |  |
| 2 | 2 |  |  |  |  |
| 3 | 3 |  |  |  |  |
| 4 | 4 |  |  |  |  |
| 5 | 4 (nueva ocupación) |  |  |  |  |

## PNF-05 Almacenamiento y frecuencia (RNF5, RNF7 y RNF8)

| Dato | Valor |
|---|---|
| `STATUS` de inicio de la ventana (S, hh:mm:ss) |  |
| `Historico: MQTT local` al inicio |  |
| `STATUS` de fin de la ventana (S, hh:mm:ss) |  |
| `Historico: MQTT local` al final |  |
| N = final − inicio |  |
| Inicio y fin del reposo del bloque 5 (S) |  |

Cálculo después de la sesión: `python tools/analizar_pruebas.py` con el CSV de PF-04, el log serie y la exportación de CloudWatch ([informe, 4.1](../../informe-practica-4.md#41-plan-de-pruebas)). Salida en `docs/evidencias/pruebas/PNF-05/analisis.md`.

## PNF-06 Reconexión y operación sin conexión (RNF6, RNF10 y E4)

Antes del primer corte: barrera cerrada, modo `automatico`, plazas libres.

| Corte | `STATUS` de corte (S) | Frases dichas (hora) | Transiciones IR durante el corte | `OPEN` bloqueado sin red (S/N) | `STATUS` de restauración (S) | `AWS MQTT/TLS conectado` (S) | Primer `AWS UPDATE` con el nuevo `modo` (S) | t_rec (s) | t_sinc (s) | ¿La barrera se movió? | Ítems recibidos / transiciones |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 |  | *"activa el modo manual"*, *"abre la barrera"* |  |  |  |  |  |  |  |  |  / 8 |
| 2 |  | *"activa el modo automático"*, *"abre la barrera"* |  | — |  |  |  |  |  |  |  / 4 |

## Observaciones y diagnóstico

| ID | Hora | Síntoma | Datos de diagnóstico (`STATUS`, tensión del servo, CloudWatch) | Acción |
|---|---|---|---|---|
|  |  |  |  |  |
|  |  |  |  |  |
