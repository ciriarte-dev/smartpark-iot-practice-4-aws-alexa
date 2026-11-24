# Análisis de las exportaciones de `SmartParkHistory`

Generado por [`analizar_historial.py`](analizar_historial.py) a partir de [`historial-barrera.csv`](../pruebas/PF-04/historial-barrera.csv) y [`historial-muestra.csv`](../pruebas/PF-04/historial-muestra.csv). Reproducción, desde la raíz del repositorio: `python docs/evidencias/analisis/analizar_historial.py > docs/evidencias/analisis/historial-resultados.md`; en Windows, con `PYTHONUTF8=1`.

## Depuración

| Paso | Ítems |
|---|---:|
| Filas de ambos CSV | 88 |
| `eventKey` repetidos entre los dos CSV | 2 |
| Ítems únicos | 86 |
| Excluidos: `eventId` con prefijo `prueba` (publicados a mano desde el cliente MQTT) | 2 |
| Eventos del firmware ([`historial-sesion.csv`](../pruebas/PNF-05/historial-sesion.csv)) | 84 |

- `prueba-mqtt-slot2-001`: `occurredAt` 2026-11-19 14:05:00, Δ0 = 2815,3 s (excluido).
- `prueba-slot1-001`: `occurredAt` 2026-11-19 14:05:00, Δ0 = 1,0 s (excluido).

## Eventos por día, tipo y origen (hora de Bolivia, UTC−4)

| Día | `eventType` | `origin` | Ítems |
|---|---|---|---:|
| 2026-11-19 | `barrier_changed` | `edge` | 2 |
| 2026-11-19 | `occupancy_changed` | `edge` | 33 |
| 2026-11-19 | `sample` | `edge` | 8 |
| 2026-11-20 | `barrier_changed` | `edge` | 1 |
| 2026-11-20 | `barrier_changed` | `legado` | 4 |
| 2026-11-20 | `occupancy_changed` | `edge` | 5 |
| 2026-11-22 | `barrier_changed` | `edge` | 4 |
| 2026-11-22 | `barrier_changed` | `legado` | 27 |

## Δ0 = `receivedAt` − `occurredAt` (s)

Criterio complementario de RNF8: Δ0 ≤ 15 s. Un evento está **en cola** si ocurrió antes de que pasaran 10 s (`HISTORY_PUBLISH_INTERVAL_MS`) desde la publicación del evento anterior del mismo arranque; si no, está **aislado**.

| Grupo | n | Mín | Mediana | Promedio | Máx | P95 | ≤ 15 s |
|---|---:|---:|---:|---:|---:|---:|---:|
| Aislados | 37 | 0,2 | 1,2 | 2,2 | 9,8 | 9,6 | 37/37 |
| En cola | 47 | 1,3 | 53,4 | 73,2 | 179,1 | 173,7 | 10/47 |
| Todos | 84 | 0,2 | 9,7 | 42,0 | 179,1 | 171,8 | 47/84 |

## Intervalos entre muestras (`sample`)

Criterio de RNF5: 300 ± 10 s (`HISTORY_SAMPLE_INTERVAL_MS`). Cada arranque publica una muestra al obtener la hora.

| Muestra | `occurredAt` | Arranque (`bootId`) | Intervalo (s) | Mismo arranque que la anterior |
|---|---|---|---:|---|
| 1 | 2026-11-19 16:32:28 | `58ffafe8` | — | — |
| 2 | 2026-11-19 16:37:01 | `9e1749cb` | 272,7 | No (arranque nuevo) |
| 3 | 2026-11-19 16:38:50 | `1b383156` | 109,3 | No (arranque nuevo) |
| 4 | 2026-11-19 16:42:18 | `563270e7` | 207,4 | No (arranque nuevo) |
| 5 | 2026-11-19 16:43:02 | `06b57c77` | 44,8 | No (arranque nuevo) |
| 6 | 2026-11-19 16:45:45 | `39cdb54e` | 162,7 | No (arranque nuevo) |
| 7 | 2026-11-19 16:50:45 | `39cdb54e` | 300,0 | Sí |
| 8 | 2026-11-19 16:55:45 | `39cdb54e` | 300,0 | Sí |

Arranques distintos entre las muestras: 6.

## Integridad reconstruida por número de secuencia

`eventId` = `<bootId>-<secuencia>`. La exportación de lecturas cubre sin filtro de tipo hasta 2026-11-20 00:56:25; solo los arranques con todos sus eventos hasta esa hora permiten contar N (secuencias generadas) frente a M (ítems almacenados). En los demás arranques solo se exportaron los eventos de barrera.

| Arranque | Primer `occurredAt` | Secuencias | N | M | M/N | Cubierto por la exportación |
|---|---|---|---:|---:|---:|---|
| `58ffafe8` | 2026-11-19 16:32:28 | 0x1–0x1 | 1 | 1 | 100,0 % | Sí |
| `9e1749cb` | 2026-11-19 16:37:01 | 0x1–0x1 | 1 | 1 | 100,0 % | Sí |
| `1b383156` | 2026-11-19 16:38:50 | 0x1–0x1 | 1 | 1 | 100,0 % | Sí |
| `563270e7` | 2026-11-19 16:42:18 | 0x1–0x1 | 1 | 1 | 100,0 % | Sí |
| `06b57c77` | 2026-11-19 16:43:02 | 0x1–0x1 | 1 | 1 | 100,0 % | Sí |
| `39cdb54e` | 2026-11-19 16:45:45 | 0x1–0x32 | 50 | 38 | 76,0 % | Sí |
|  | Ausentes: 0x1e, 0x20, 0x21, 0x24, 0x25, 0x26, 0x29, 0x2a, 0x2d, 0x2e, 0x2f, 0x30 |  |  |  |  |  |
| `df03561b` | 2026-11-20 00:56:18 | 0x1–0x6 | 6 | 5 | 83,3 % | Sí |
|  | Ausentes: 0x1 |  |  |  |  |  |
| `443500e0` | 2026-11-20 07:36:46 | 0x2–0x2 | — | 1 | — | No (solo barrera) |
| `9b33b8ed` | 2026-11-20 11:07:22 | 0x2–0x5 | — | 4 | — | No (solo barrera) |
| `cf9c4b74` | 2026-11-22 17:49:35 | 0x6–0x7 | — | 2 | — | No (solo barrera) |
| `3deae424` | 2026-11-22 18:57:59 | 0x2c–0x2d | — | 2 | — | No (solo barrera) |
| `8833b560` | 2026-11-22 19:30:52 | 0x59–0x60 | — | 8 | — | No (solo barrera) |
| `77beba5a` | 2026-11-22 19:59:45 | 0x2–0x5 | — | 4 | — | No (solo barrera) |
| `f50ed1bf` | 2026-11-22 20:31:58 | 0x2–0x10 | — | 5 | — | No (solo barrera) |
| `92f13203` | 2026-11-22 21:46:58 | 0x32–0x65 | — | 4 | — | No (solo barrera) |
| `44c8d88b` | 2026-11-22 23:27:18 | 0x1c–0x34 | — | 6 | — | No (solo barrera) |
| **Arranques cubiertos** |  |  | **61** | **48** | **78,7 %** |  |

## Detalle por evento

| `occurredAt` | `eventType` | `origin` | Arranque | Δ0 (s) | Grupo |
|---|---|---|---|---:|---|
| 2026-11-19 16:32:28 | `sample` | `edge` | `58ffafe8` | 2,5 | aislado |
| 2026-11-19 16:37:01 | `sample` | `edge` | `9e1749cb` | 2,5 | aislado |
| 2026-11-19 16:38:50 | `sample` | `edge` | `1b383156` | 2,6 | aislado |
| 2026-11-19 16:42:18 | `sample` | `edge` | `563270e7` | 2,5 | aislado |
| 2026-11-19 16:43:02 | `sample` | `edge` | `06b57c77` | 2,3 | aislado |
| 2026-11-19 16:45:45 | `sample` | `edge` | `39cdb54e` | 2,5 | aislado |
| 2026-11-19 16:50:45 | `sample` | `edge` | `39cdb54e` | 0,2 | aislado |
| 2026-11-19 16:55:45 | `sample` | `edge` | `39cdb54e` | 0,2 | aislado |
| 2026-11-19 16:56:34 | `occupancy_changed` | `edge` | `39cdb54e` | 0,2 | aislado |
| 2026-11-19 16:56:35 | `occupancy_changed` | `edge` | `39cdb54e` | 10,0 | en cola |
| 2026-11-19 16:57:27 | `occupancy_changed` | `edge` | `39cdb54e` | 0,2 | aislado |
| 2026-11-19 16:57:30 | `occupancy_changed` | `edge` | `39cdb54e` | 7,5 | en cola |
| 2026-11-19 16:57:31 | `occupancy_changed` | `edge` | `39cdb54e` | 16,5 | en cola |
| 2026-11-19 16:57:32 | `occupancy_changed` | `edge` | `39cdb54e` | 25,3 | en cola |
| 2026-11-19 16:57:32 | `occupancy_changed` | `edge` | `39cdb54e` | 35,1 | en cola |
| 2026-11-19 16:57:32 | `occupancy_changed` | `edge` | `39cdb54e` | 44,9 | en cola |
| 2026-11-19 16:57:34 | `occupancy_changed` | `edge` | `39cdb54e` | 53,4 | en cola |
| 2026-11-19 16:57:45 | `occupancy_changed` | `edge` | `39cdb54e` | 51,9 | en cola |
| 2026-11-19 16:57:47 | `occupancy_changed` | `edge` | `39cdb54e` | 60,5 | en cola |
| 2026-11-19 16:57:48 | `occupancy_changed` | `edge` | `39cdb54e` | 69,0 | en cola |
| 2026-11-19 16:57:49 | `occupancy_changed` | `edge` | `39cdb54e` | 78,0 | en cola |
| 2026-11-19 16:57:51 | `occupancy_changed` | `edge` | `39cdb54e` | 86,6 | en cola |
| 2026-11-19 16:57:52 | `occupancy_changed` | `edge` | `39cdb54e` | 95,8 | en cola |
| 2026-11-19 16:58:19 | `occupancy_changed` | `edge` | `39cdb54e` | 78,8 | en cola |
| 2026-11-19 16:58:20 | `occupancy_changed` | `edge` | `39cdb54e` | 87,8 | en cola |
| 2026-11-19 16:58:21 | `occupancy_changed` | `edge` | `39cdb54e` | 96,6 | en cola |
| 2026-11-19 16:58:22 | `occupancy_changed` | `edge` | `39cdb54e` | 105,8 | en cola |
| 2026-11-19 16:58:23 | `occupancy_changed` | `edge` | `39cdb54e` | 114,6 | en cola |
| 2026-11-19 16:58:24 | `occupancy_changed` | `edge` | `39cdb54e` | 123,9 | en cola |
| 2026-11-19 16:58:40 | `occupancy_changed` | `edge` | `39cdb54e` | 137,2 | en cola |
| 2026-11-19 16:58:41 | `occupancy_changed` | `edge` | `39cdb54e` | 146,9 | en cola |
| 2026-11-19 16:58:41 | `occupancy_changed` | `edge` | `39cdb54e` | 156,8 | en cola |
| 2026-11-19 16:58:44 | `occupancy_changed` | `edge` | `39cdb54e` | 164,1 | en cola |
| 2026-11-19 16:58:46 | `occupancy_changed` | `edge` | `39cdb54e` | 171,8 | en cola |
| 2026-11-19 16:58:52 | `occupancy_changed` | `edge` | `39cdb54e` | 175,5 | en cola |
| 2026-11-19 16:59:07 | `occupancy_changed` | `edge` | `39cdb54e` | 170,9 | en cola |
| 2026-11-19 16:59:09 | `occupancy_changed` | `edge` | `39cdb54e` | 179,1 | en cola |
| 2026-11-19 16:59:31 | `occupancy_changed` | `edge` | `39cdb54e` | 166,6 | en cola |
| 2026-11-19 16:59:34 | `occupancy_changed` | `edge` | `39cdb54e` | 173,7 | en cola |
| 2026-11-19 16:59:53 | `occupancy_changed` | `edge` | `39cdb54e` | 164,5 | en cola |
| 2026-11-19 16:59:56 | `occupancy_changed` | `edge` | `39cdb54e` | 172,2 | en cola |
| 2026-11-19 17:00:34 | `barrier_changed` | `edge` | `39cdb54e` | 3,4 | en cola |
| 2026-11-19 17:00:37 | `barrier_changed` | `edge` | `39cdb54e` | 10,3 | en cola |
| 2026-11-20 00:56:18 | `occupancy_changed` | `edge` | `df03561b` | 1,4 | aislado |
| 2026-11-20 00:56:19 | `occupancy_changed` | `edge` | `df03561b` | 11,2 | en cola |
| 2026-11-20 00:56:20 | `occupancy_changed` | `edge` | `df03561b` | 19,7 | en cola |
| 2026-11-20 00:56:22 | `occupancy_changed` | `edge` | `df03561b` | 27,7 | en cola |
| 2026-11-20 00:56:25 | `occupancy_changed` | `edge` | `df03561b` | 34,3 | en cola |
| 2026-11-20 07:36:46 | `barrier_changed` | `edge` | `443500e0` | 2,1 | aislado |
| 2026-11-20 11:07:22 | `barrier_changed` | `legado` | `9b33b8ed` | 0,2 | aislado |
| 2026-11-20 11:07:30 | `barrier_changed` | `legado` | `9b33b8ed` | 2,0 | en cola |
| 2026-11-20 11:08:27 | `barrier_changed` | `legado` | `9b33b8ed` | 0,2 | aislado |
| 2026-11-20 11:08:47 | `barrier_changed` | `legado` | `9b33b8ed` | 0,2 | aislado |
| 2026-11-22 17:49:35 | `barrier_changed` | `legado` | `cf9c4b74` | 0,2 | aislado |
| 2026-11-22 17:49:44 | `barrier_changed` | `legado` | `cf9c4b74` | 1,3 | en cola |
| 2026-11-22 18:57:59 | `barrier_changed` | `edge` | `3deae424` | 7,8 | aislado |
| 2026-11-22 18:58:01 | `barrier_changed` | `edge` | `3deae424` | 15,9 | en cola |
| 2026-11-22 19:30:52 | `barrier_changed` | `legado` | `8833b560` | 6,3 | aislado |
| 2026-11-22 19:31:05 | `barrier_changed` | `legado` | `8833b560` | 2,7 | en cola |
| 2026-11-22 19:31:28 | `barrier_changed` | `legado` | `8833b560` | 9,8 | aislado |
| 2026-11-22 19:31:35 | `barrier_changed` | `legado` | `8833b560` | 13,0 | en cola |
| 2026-11-22 19:31:43 | `barrier_changed` | `legado` | `8833b560` | 15,5 | en cola |
| 2026-11-22 19:31:51 | `barrier_changed` | `legado` | `8833b560` | 17,4 | en cola |
| 2026-11-22 19:31:58 | `barrier_changed` | `legado` | `8833b560` | 20,3 | en cola |
| 2026-11-22 19:32:06 | `barrier_changed` | `legado` | `8833b560` | 21,8 | en cola |
| 2026-11-22 19:59:45 | `barrier_changed` | `legado` | `77beba5a` | 9,6 | aislado |
| 2026-11-22 20:00:13 | `barrier_changed` | `legado` | `77beba5a` | 0,3 | aislado |
| 2026-11-22 20:00:25 | `barrier_changed` | `legado` | `77beba5a` | 0,3 | aislado |
| 2026-11-22 20:03:33 | `barrier_changed` | `legado` | `77beba5a` | 0,2 | aislado |
| 2026-11-22 20:31:58 | `barrier_changed` | `legado` | `f50ed1bf` | 0,2 | aislado |
| 2026-11-22 20:32:17 | `barrier_changed` | `legado` | `f50ed1bf` | 0,3 | aislado |
| 2026-11-22 20:33:18 | `barrier_changed` | `legado` | `f50ed1bf` | 1,9 | aislado |
| 2026-11-22 20:39:37 | `barrier_changed` | `edge` | `f50ed1bf` | 0,3 | aislado |
| 2026-11-22 20:39:50 | `barrier_changed` | `edge` | `f50ed1bf` | 0,2 | aislado |
| 2026-11-22 21:46:58 | `barrier_changed` | `legado` | `92f13203` | 2,4 | aislado |
| 2026-11-22 21:47:06 | `barrier_changed` | `legado` | `92f13203` | 3,5 | en cola |
| 2026-11-22 21:47:39 | `barrier_changed` | `legado` | `92f13203` | 1,2 | aislado |
| 2026-11-22 21:48:01 | `barrier_changed` | `legado` | `92f13203` | 8,6 | aislado |
| 2026-11-22 23:27:18 | `barrier_changed` | `legado` | `44c8d88b` | 0,5 | aislado |
| 2026-11-22 23:30:08 | `barrier_changed` | `legado` | `44c8d88b` | 1,1 | aislado |
| 2026-11-22 23:31:23 | `barrier_changed` | `legado` | `44c8d88b` | 0,3 | aislado |
| 2026-11-22 23:32:00 | `barrier_changed` | `legado` | `44c8d88b` | 4,0 | aislado |
| 2026-11-22 23:33:19 | `barrier_changed` | `legado` | `44c8d88b` | 4,7 | aislado |
| 2026-11-22 23:33:41 | `barrier_changed` | `legado` | `44c8d88b` | 3,0 | aislado |
