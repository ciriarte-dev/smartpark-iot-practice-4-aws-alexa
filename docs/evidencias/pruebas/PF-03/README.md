# PF-03 — Vinculación por PIN

Respalda RF7, RF8, RF9. Procedimiento, resultado esperado y criterio: [PF-03 en 4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales). Los valores se anotan primero en la [hoja de registro](../../hojas-registro/hoja-pruebas-funcionales.md).

| Archivo esperado | Cómo obtenerlo |
|---|---|
| `simulador-*.png` | Capturas del simulador de Alexa (**Test → Development**) con la respuesta y el panel **JSON Input** de cada frase. |
| `bindings-*.png` | Captura del ítem del usuario en la tabla `SmartParkBindings` (consola de DynamoDB) en cada parte: A (creado), B (`revision` nueva) y C (eliminado). |

Las rutas con `../` apuntan a archivos compartidos con otras pruebas. Esta carpeta se completa durante la sesión de banco; no contiene resultados todavía.
