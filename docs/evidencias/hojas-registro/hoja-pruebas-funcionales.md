# Hoja de registro — pruebas funcionales (Práctica 4)

Imprimir antes de la sesión y completar a mano. Procedimiento y resultado esperado de cada prueba: [informe, sección 4.2](../../informe-practica-4.md#42-pruebas-funcionales).

- Fecha: ____________  Responsable: ____________________  Testigo: ____________________
- Firmware (commit): ________  Thing: `smartpark-grupo9-01`  Región: `us-east-1`
- Reloj de referencia: hora del PC sincronizada por NTP (monitor serie con `pio device monitor -f time`). Anotar la hora con segundos (hh:mm:ss).
- Evidencia: guardar capturas y videos en `docs/evidencias/pruebas/<ID>/` con el nombre `<ID>-<repetición>-<descripción>`.

## Regresión RF1–RF9 y RF10–RF15

| ID | Rep. | Fecha | Hora (hh:mm:ss) | Acción realizada | Valor observado (Shadow / DynamoDB / respuesta) | ¿Cumple? (S/N) | Archivo de evidencia | Observación |
|---|---|---|---|---|---|---|---|---|
| PF-01 | 1 |  |  |  |  |  |  |  |
| PF-02 | 1 |  |  |  |  |  |  |  |
| PF-02 | 2 |  |  |  |  |  |  |  |
| PF-03 | 1 |  |  |  |  |  |  |  |
| PF-03 | 2 |  |  |  |  |  |  |  |
| PF-03 | 3 |  |  |  |  |  |  |  |
| PF-04 | 1 |  |  |  |  |  |  |  |
| PF-04 | 2 |  |  |  |  |  |  |  |
| PF-05 | 1 |  |  |  |  |  |  |  |
| PF-05 | 2 |  |  |  |  |  |  |  |
| PF-06 | 1 |  |  |  |  |  |  |  |
| PF-06 | 2 |  |  |  |  |  |  |  |
| PF-07 | 1 |  |  |  |  |  |  |  |
| PF-07 | 2 |  |  |  |  |  |  |  |
| PF-08 | 1 |  |  |  |  |  |  |  |
| PF-08 | 2 |  |  |  |  |  |  |  |
| PF-08 | 3 |  |  |  |  |  |  |  |
| PF-08 | 4 |  |  |  |  |  |  |  |
| PF-09 | 1 |  |  |  |  |  |  |  |
| PF-10 | 1 |  |  |  |  |  |  |  |
| PF-10 | 2 |  |  |  |  |  |  |  |
| PF-10 | 3 |  |  |  |  |  |  |  |
| PF-11 | 1 |  |  |  |  |  |  |  |
| PF-11 | 2 |  |  |  |  |  |  |  |
| PF-11 | 3 |  |  |  |  |  |  |  |
| PF-11 | 4 |  |  |  |  |  |  |  |
| PF-12 | 1 |  |  |  |  |  |  |  |
| PF-12 | 2 |  |  |  |  |  |  |  |
| PF-12 | 3 |  |  |  |  |  |  |  |
| PF-13 | 1 |  |  |  |  |  |  |  |
| PF-13 | 2 |  |  |  |  |  |  |  |
| PF-14 | 1 |  |  |  |  |  |  |  |
| PF-16 | 1 |  |  |  |  |  |  |  |
| PF-16 | 2 |  |  |  |  |  |  |  |
| PF-17 | 1 |  |  |  |  |  |  |  |
| PF-17 | 2 |  |  |  |  |  |  |  |
| PF-17 | 3 |  |  |  |  |  |  |  |
| PF-17 | 4 |  |  |  |  |  |  |  |

## PF-15 — frases de la skill en el simulador (RF14 y consultas de plazas)

En **Test → JSON Input** anotar el intent reconocido y el valor del slot. Con cuenta vinculada y ESP32 conectado.

| # | Frase pronunciada o escrita | Intent esperado | Intent obtenido | Slot obtenido | Respuesta de Alexa (resumen) | Shadow (`desired` / `reported`) | S/N |
|---|---|---|---|---|---|---|---|
| 1 | cuántos espacios hay disponibles | `ConsultarDisponiblesIntent` |  |  |  |  |  |
| 2 | consulta los espacios libres | `ConsultarDisponiblesIntent` |  |  |  |  |  |
| 3 | cuántas plazas libres quedan | `ConsultarDisponiblesIntent` |  |  |  |  |  |
| 4 | cuántos espacios están ocupados | `ConsultarOcupadosIntent` |  |  |  |  |  |
| 5 | consulta los espacios ocupados | `ConsultarOcupadosIntent` |  |  |  |  |  |
| 6 | cuántas plazas están ocupadas | `ConsultarOcupadosIntent` |  |  |  |  |  |
| 7 | cómo está el estacionamiento dos | `ConsultarEstacionamientoIntent` |  |  |  |  |  |
| 8 | está libre la plaza cuatro | `ConsultarEstacionamientoIntent` |  |  |  |  |  |
| 9 | consulta el espacio uno | `ConsultarEstacionamientoIntent` |  |  |  |  |  |
| 10 | consulta el modo del parqueo | `ConsultarModoIntent` |  |  |  |  |  |
| 11 | en qué modo está el parqueo | `ConsultarModoIntent` |  |  |  |  |  |
| 12 | cuál es el modo actual | `ConsultarModoIntent` |  |  |  |  |  |
| 13 | activa el modo automático | `ActivarModoAutomaticoIntent` |  |  |  |  |  |
| 14 | pon el parqueo en modo automático | `ActivarModoAutomaticoIntent` |  |  |  |  |  |
| 15 | cambia a modo automático | `ActivarModoAutomaticoIntent` |  |  |  |  |  |
| 16 | activa el modo manual | `ActivarModoManualIntent` |  |  |  |  |  |
| 17 | pon el parqueo en modo manual | `ActivarModoManualIntent` |  |  |  |  |  |
| 18 | cambia a modo manual | `ActivarModoManualIntent` |  |  |  |  |  |
| 19 | consulta el antirrebote | `ConsultarAntirreboteIntent` |  |  |  |  |  |
| 20 | cuántos milisegundos tiene el antirrebote | `ConsultarAntirreboteIntent` |  |  |  |  |  |
| 21 | cuál es el valor del antirrebote | `ConsultarAntirreboteIntent` |  |  |  |  |  |
| 22 | ajusta el antirrebote a ciento veinte milisegundos | `CambiarAntirreboteIntent` |  |  |  |  |  |
| 23 | cambia el antirrebote a doscientos | `CambiarAntirreboteIntent` |  |  |  |  |  |
| 24 | configura cincuenta milisegundos de antirrebote | `CambiarAntirreboteIntent` |  |  |  |  |  |
| 25 | cambia el antirrebote a cuarenta y nueve (límite inferior − 1) | `CambiarAntirreboteIntent`: rechazo |  |  |  |  |  |
| 26 | cambia el antirrebote a mil (límite superior) | `CambiarAntirreboteIntent` |  |  |  |  |  |
| 27 | cambia el antirrebote a mil uno (límite superior + 1) | `CambiarAntirreboteIntent`: rechazo |  |  |  |  |  |
| 28 | consulta el espacio cinco (fuera de rango) | `ConsultarEstacionamientoIntent`: rechazo |  |  |  |  |  |
