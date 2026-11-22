# PNF-06 — Reconexión y operación sin conexión

Respalda RNF6, RNF10, RF13 (E4). Procedimiento, resultado esperado y criterio: [PNF-06 en 4.3 del informe](../../../informe-practica-4.md#43-pruebas-no-funcionales). Los valores se anotan primero en la [hoja de registro](../../hojas-registro/hoja-pruebas-no-funcionales.md).

| Archivo esperado | Cómo obtenerlo |
|---|---|
| `../serial-sesion.txt` (log común de la sesión) | Log del monitor serie de toda la sesión: `pio device monitor -f time \| tee docs/evidencias/pruebas/serial-sesion.txt` en Git Bash, iniciado en el bloque 0 (4.1). |
| `update-accepted-*.png` | Captura del cliente de prueba MQTT de AWS IoT suscrito a `$aws/things/smartpark-grupo9-01/shadow/update/accepted` al restaurar la red en cada corte (nuevo `modo` reportado). |
| `../PF-04/historial-sesion.csv` | Se exporta en PF-04: consola de DynamoDB → `SmartParkHistory` → *Query* por `thingName` y rango de `eventKey` de la sesión → *Download results to CSV*. |

Las rutas con `../` apuntan a archivos compartidos con otras pruebas. Esta carpeta se completa durante la sesión de banco; no contiene resultados todavía.
