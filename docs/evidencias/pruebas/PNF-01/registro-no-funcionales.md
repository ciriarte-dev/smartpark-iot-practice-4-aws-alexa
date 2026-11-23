# Transcripción de `registro-no-funcionales.pdf`

Transcripción literal de [`registro-no-funcionales.pdf`](registro-no-funcionales.pdf) (hoja de registro de la sesión de banco del 22/11/2026). Los valores se copian tal cual; las filas marcadas *Calculado* no están en el PDF y se obtienen de los valores individuales. El PDF no se modifica.

Los títulos del PDF no siguen la numeración del informe: la tabla titulada **"PNF-04 — Reconexión Wi-Fi y MQTT"** corresponde a la prueba **PNF-06** del informe (RNF6). La prueba PNF-04 del informe (IR → LED, RNF9) no tiene tabla en el PDF.

## PNF-01 — Estabilidad (RNF1)

| Duración (min) | Reinicios | Bloqueos | Desconexiones | UPDATE aceptados | Estado |
|---:|---:|---:|---:|---:|---|
| 10 | 3 | 0 | 1 | 10 | (vacío) |

Criterio del informe (1.4): ≥ 15 min con 0 reinicios, 0 bloqueos y 0 reconexiones no provocadas.

## PNF-02 — Exactitud del HC-SR04 (RNF2)

| Distancia de referencia (cm) | Lectura 1 | Lectura 2 | Lectura 3 | Lectura 4 | Lectura 5 | Promedio (cm) | Error absoluto máximo (cm) |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 5 | 5.2 | 5.1 | 5.0 | 5.2 | 5.3 | 5.2 | 0.3 |
| 10 | 9.9 | 10.0 | 9.8 | 10.1 | 10.0 | 10.0 | 0.2 |
| 15 | 15.0 | 15.2 | 14.9 | 15.1 | 15.0 | 15.0 | 0.2 |
| 20 | 21.0 | 20.4 | 20.8 | 20.2 | 20.6 | 20.6 | 1.0 |
| 30 | 29.5 | 30.1 | 29.8 | 30.2 | 29.7 | 29.9 | 0.5 |
| 40 | 40.5 | 40.2 | 39.8 | 40.4 | 40.1 | 40.2 | 0.5 |
| 50 | 50.1 | 50.5 | 49.7 | 50.3 | 49.9 | 50.1 | 0.5 |
| *Calculado: 10, 20 y 30 cm (15 lecturas)* | | | | | | | *1,0 (a 20 cm)* |

El PDF no contiene mediciones de los sensores IR (RNF3).

## PNF-03 (PDF) — Tiempo de respuesta de Alexa (RNF4)

| Medición | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | Mínimo | Máximo | Promedio |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Tiempo (s) | 2 | 3 | 1 | 2 | 4 | 1 | 1 | 1 | 1 | 3 | 1 | 5 | 1.9 |
| *Calculado de las 10 mediciones* | | | | | | | | | | | *1* | *4* | *1,9* |

**Nota de transcripción.** El PDF anota 5 s como máximo, pero ninguna de las 10 mediciones vale 5 s: el máximo de los valores individuales es 4 s (medición 5). El informe usa los valores individuales. Mediana calculada: 1,5 s; 9 de 10 mediciones ≤ 3,0 s. Los tiempos están en segundos enteros (cronómetro), sin conteo de cuadros de video.

## PNF-04 (PDF) = PNF-06 del informe — Reconexión Wi-Fi y MQTT (RNF6)

| Corte | Tiempo hasta Wi-Fi (s) | Tiempo hasta MQTT (s) | ¿Se aplicó el `desired` pendiente? |
|---:|---|---|---|
| 1 | 2 min | 1 min | Sí |
| 2 | 1 min | 3 min | Sí |
| 3 | 4 min | 1 min | Sí |

**Nota de transcripción.** El encabezado dice segundos, pero los valores están anotados en minutos enteros. El PDF no registra la duración de cada corte ni desde qué instante se cuenta cada tiempo.
