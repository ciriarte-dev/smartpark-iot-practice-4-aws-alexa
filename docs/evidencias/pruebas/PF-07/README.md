# PF-07 — Consultas y cambios de modo y antirrebote por voz

Respalda RF14, RF3. Procedimiento, resultado esperado y criterio: [PF-07 en 4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales). Los valores se anotan primero en la [hoja de registro](../../hojas-registro/hoja-pruebas-funcionales.md).

| Archivo esperado | Cómo obtenerlo |
|---|---|
| `simulador-*.png` | Capturas del simulador de Alexa (**Test → Development**) con la respuesta y el panel **JSON Input** de cada frase. |
| `update-accepted.png` | Captura del cliente de prueba MQTT de AWS IoT suscrito a `$aws/things/smartpark-grupo9-01/shadow/update/accepted` tras *"ajusta el antirrebote a 120 milisegundos"*. |

Las rutas con `../` apuntan a archivos compartidos con otras pruebas. Esta carpeta se completa durante la sesión de banco; no contiene resultados todavía.
