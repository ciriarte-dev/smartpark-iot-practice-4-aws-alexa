# PF-05 — Descarte de inválidos y duplicados

Respalda RF11. Procedimiento, resultado esperado y criterio: [PF-05 en 4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales). Los valores se anotan primero en la [hoja de registro](../../hojas-registro/hoja-pruebas-funcionales.md).

| Archivo esperado | Cómo obtenerlo |
|---|---|
| [`mensajes.txt`](mensajes.txt) | Cuerpos JSON listos para publicar en el cliente de prueba MQTT (4 inválidos) y espacio para pegar el mensaje real que se publica dos veces. |
| `../PF-04/cloudwatch.csv` | Se exporta en PF-04: CloudWatch Logs Insights sobre `/aws/lambda/SmartParkHistoryProcessor` con `fields @timestamp, @message \| sort @timestamp asc` → *Export results* → CSV. |
| `../PF-04/historial-sesion.csv` | Se exporta en PF-04: consola de DynamoDB → `SmartParkHistory` → *Query* por `thingName` y rango de `eventKey` de la sesión → *Download results to CSV*. |

Las rutas con `../` apuntan a archivos compartidos con otras pruebas. Esta carpeta se completa durante la sesión de banco; no contiene resultados todavía.
