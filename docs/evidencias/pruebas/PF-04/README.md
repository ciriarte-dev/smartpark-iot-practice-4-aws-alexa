# PF-04 — Almacenamiento del histórico

Respalda RF10, RF11, RF12. Procedimiento, resultado esperado y criterio: [PF-04 en 4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales). Los valores se anotan primero en la [hoja de registro](../../hojas-registro/hoja-pruebas-funcionales.md).

| Archivo esperado | Cómo obtenerlo |
|---|---|
| `historial-sesion.csv` | Consola de DynamoDB → `SmartParkHistory` → *Query* por `thingName` y rango de `eventKey` de la sesión → *Download results to CSV*. |
| `cloudwatch.csv` | CloudWatch Logs Insights sobre `/aws/lambda/SmartParkHistoryProcessor` con `fields @timestamp, @message \| sort @timestamp asc` → *Export results* → CSV. |

Las rutas con `../` apuntan a archivos compartidos con otras pruebas. Esta carpeta se completa durante la sesión de banco; no contiene resultados todavía.
