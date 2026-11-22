# PF-02 — Barrera y consultas por voz

Respalda RF3, RF4, RF5, RF6. Procedimiento, resultado esperado y criterio: [PF-02 en 4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales). Los valores se anotan primero en la [hoja de registro](../../hojas-registro/hoja-pruebas-funcionales.md).

| Archivo esperado | Cómo obtenerlo |
|---|---|
| `simulador-*.png` | Capturas del simulador de Alexa (**Test → Development**) con la respuesta y el panel **JSON Input** de cada frase. |
| `update-accepted.png` | Captura del cliente de prueba MQTT de AWS IoT suscrito a `$aws/things/smartpark-grupo9-01/shadow/update/accepted` tras la última orden de barrera. |
| `../serial-sesion.txt` (log común de la sesión) | Log del monitor serie de toda la sesión: `pio device monitor -f time \| tee docs/evidencias/pruebas/serial-sesion.txt` en Git Bash, iniciado en el bloque 0 (4.1). |

Las rutas con `../` apuntan a archivos compartidos con otras pruebas. Esta carpeta se completa durante la sesión de banco; no contiene resultados todavía.
