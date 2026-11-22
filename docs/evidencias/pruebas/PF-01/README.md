# PF-01 — Lectura de sensores y reporte al Shadow

Respalda RF1, RF2. Procedimiento, resultado esperado y criterio: [PF-01 en 4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales). Los valores se anotan primero en la [hoja de registro](../../hojas-registro/hoja-pruebas-funcionales.md).

| Archivo esperado | Cómo obtenerlo |
|---|---|
| `../serial-sesion.txt` (log común de la sesión) | Log del monitor serie de toda la sesión: `pio device monitor -f time \| tee docs/evidencias/pruebas/serial-sesion.txt` en Git Bash, iniciado en el bloque 0 (4.1). |
| `update-accepted.png` | Captura del cliente de prueba MQTT de AWS IoT suscrito a `$aws/things/smartpark-grupo9-01/shadow/update/accepted` tras cubrir y liberar el IR de la plaza 1. |

Las rutas con `../` apuntan a archivos compartidos con otras pruebas. Esta carpeta se completa durante la sesión de banco; no contiene resultados todavía.
