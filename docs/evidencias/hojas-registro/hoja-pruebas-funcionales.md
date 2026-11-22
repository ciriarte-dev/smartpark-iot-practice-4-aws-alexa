# Hoja de registro — pruebas funcionales (Práctica 4)

Imprimir antes de la sesión y completar a mano. Procedimiento, resultado esperado y orden de los bloques: [informe, secciones 4.1 y 4.2](../../informe-practica-4.md#42-pruebas-funcionales).

- Fecha: ____________  Responsables (maqueta / consola / registro): ______________________________
- Firmware (commit): ________  Thing: `smartpark-grupo9-01`  Región: `us-east-1`
- Log serie: `pio device monitor -f time | tee docs/evidencias/pruebas/serial-sesion.txt` (Git Bash). Hora de inicio del log: ________
- Hora: la del PC (hh:mm:ss), la misma que aparece en el log serie.
- Evidencia: `docs/evidencias/pruebas/<ID>/`, con el nombre `<ID>-<paso>-<descripción>`.

## Registro por prueba

| ID | Bloque | Paso | Hora (hh:mm:ss) | Valor observado (Serial / Shadow / DynamoDB / respuesta) | ¿Cumple? (S/N) | Archivo de evidencia |
|---|---|---|---|---|---|---|
| PF-01 | 2 | `DISTANCE` y `SLOT1` |  |  |  |  |
| PF-01 | 2 | Cubrir y liberar el IR de la plaza 1; `update/accepted` |  |  |  |  |
| PF-02 | 1 | Abrir / cerrar ×3 (video PNF-03) |  |  |  |  |
| PF-02 | 1 | *"consulta la distancia"* |  |  |  |  |
| PF-02 | 1 | *"hay un auto esperando"* |  |  |  |  |
| PF-02 | 1 | *"consulta el estado de la barrera"* |  |  |  |  |
| PF-03 | 1 | A: skill sin vinculación |  |  |  |  |
| PF-03 | 1 | A: PIN 1 generado (hora) y *"vincula mi parqueo con el codigo …"* |  |  |  |  |
| PF-03 | 1 | A: cerrar y reabrir la skill |  |  |  |  |
| PF-03 | 3 | B: PIN 2 (≥ 300 s después del PIN 1) y *"cambia mi vinculacion con el código …"* |  |  |  |  |
| PF-03 | 5 | C: *"desvincula mi parqueo"* y *"consulta la distancia"* |  |  |  |  |
| PF-04 | 1 | `OPEN` y `CLOSE` por Serial |  |  |  |  |
| PF-04 | 5 | `START` en CloudWatch dentro de la ventana de PNF-05 = N |  |  |  |  |
| PF-04 | 5 | Ítems y atributos; `origin` de cada `barrier_changed`; CSV exportado |  |  |  |  |
| PF-05 | 2 | `distancia_cm: 999` |  |  |  |  |
| PF-05 | 2 | `capacidad: 5` |  |  |  |  |
| PF-05 | 2 | `eventType: "door_opened"` |  |  |  |  |
| PF-05 | 2 | `origin: "voz"` en `sample` |  |  |  |  |
| PF-05 | 2 | Duplicado publicado dos veces |  |  |  |  |
| PF-06 | 3 | *"activa el modo automático"*; ocupar plazas 1–4. LCD esperada: `Libres:0/4 Oc:4` / `APERTURA BLOQ.` |  |  |  |  |
| PF-06 | 3 | *"abre la barrera"* con 4 ocupadas (E2). LCD: `APERTURA BLOQ.` |  |  |  |  |
| PF-06 | 3 | `OPEN` con 4 ocupadas (E2). LCD: `APERTURA BLOQ.` |  |  |  |  |
| PF-06 | 3 | *"cierra la barrera"* (E3). LCD: `APERTURA BLOQ.` |  |  |  |  |
| PF-06 | 3 | Liberar la plaza 4 y *"abre la barrera"*. LCD: `Libres:1/4 Oc:3`; fila 1 alterna cada 2 s `Modo: automatico` / `Barrera: abierta` |  |  |  |  |
| PF-06 | 3 | `CLOSE`; ocupar la plaza 4; *"activa el modo manual"* y *"abre la barrera"*. LCD: `Libres:0/4 Oc:4` / `PARQUEO LLENO` |  |  |  |  |
| PF-08 | 5 | Consulta M1 |  |  |  |  |
| PF-08 | 5 | Consulta M2 |  |  |  |  |
| PF-08 | 5 | Consulta M3 |  |  |  |  |
| PF-08 | 5 | Consulta M4 |  |  |  |  |

## PF-07 — frases de RF14 (bloque 3)

Modelo: [`alexa/es-US.smartpark-v2.final.json`](../../../alexa/es-US.smartpark-v2.final.json). Estado inicial: `manual`, 4 plazas ocupadas. En **JSON Input** anotar el intent y el valor del slot.

| # | Frase | Intent esperado | Slot | Respuesta / Shadow esperado | Intent y slot observados | Respuesta observada | ¿Cumple? (S/N) |
|---|---|---|---|---|---|---|---|
| 1 | cuántos espacios hay disponibles | `ConsultarDisponiblesIntent` | — | 0 de 4 |  |  |  |
| 2 | cuántas plazas están ocupadas | `ConsultarOcupadosIntent` | — | 4 de 4 |  |  |  |
| 3 | está libre la plaza 2 | `ConsultarEstacionamientoIntent` | `espacio` = 2 | ocupado |  |  |  |
| 4 | está libre la plaza 5 | `ConsultarEstacionamientoIntent` | `espacio` = 5 | *"Dime un número de estacionamiento del uno al cuatro."* |  |  |  |
| 5 | en qué modo está el parqueo | `ConsultarModoIntent` | — | manual |  |  |  |
| 6 | consulta el antirrebote | `ConsultarAntirreboteIntent` | — | 120 milisegundos |  |  |  |
| 7 | ajusta el antirrebote a 200 milisegundos | `CambiarAntirreboteIntent` | `milisegundos` = 200 | `desired` y luego `reported` = 200 |  |  |  |
| 8 | ajusta el antirrebote a 1001 milisegundos | `CambiarAntirreboteIntent` | `milisegundos` = 1001 | *"Dime un valor entero entre 50 y 1000 milisegundos."*; Shadow sin cambio |  |  |  |
| 9 | ajusta el antirrebote a 120 milisegundos | `CambiarAntirreboteIntent` | `milisegundos` = 120 | `reported` = 120 |  |  |  |
| 10 | ayuda | `AMAZON.HelpIntent` | — | ayuda; la sesión sigue abierta |  |  |  |
| 11 | cancela | `AMAZON.CancelIntent` | — | *"Hasta luego."* |  |  |  |
| 12 | detente (tras reabrir la skill) | `AMAZON.StopIntent` | — | *"Hasta luego."* |  |  |  |

## Observaciones

| ID | Hora | Observación |
|---|---|---|
|  |  |  |
|  |  |  |
