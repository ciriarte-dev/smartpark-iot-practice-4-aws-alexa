# PF-01 — Lectura de sensores y reporte al Shadow

Respalda RF1, RF2. Procedimiento, resultado esperado y criterio: [PF-01 en 4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales). Los valores se anotan primero en la [hoja de registro](../../hojas-registro/hoja-pruebas-funcionales.md).

## Evidencia adjunta

- [Video de la sesión PF-01](<WhatsApp Video 2026-11-22 at 19.32.05 (1).mp4>)

## Resultado registrado

| Paso | Hora | Resultado observado | ¿Cumple? |
|---|---|---|---|
| Enviar `DISTANCE` y `SLOT1` | — | Los comandos se ejecutaron | Sí |
| Cubrir y liberar el IR de la plaza 1; revisar `update/accepted` | 19:24:41 (UTC-04:00) | La plaza cambió el estado de libre a ocupado | Sí |

## Estado

**Parcialmente ejecutada:** los pasos registrados cumplen, pero todavía pueden adjuntarse como evidencia separada el log serie y la captura `update/accepted.png`.

## Archivos pendientes o de respaldo

| Archivo | Evidencia |
|---|---|
| `../serial-sesion.txt` | Log común del monitor serie iniciado antes de la sesión. |
| `update-accepted.png` | Captura MQTT suscrita a `$aws/things/smartpark-grupo9-01/shadow/update/accepted`. |
