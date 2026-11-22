# PNF-02 — Exactitud del HC-SR04 y de los IR

Respalda RNF2, RNF3. Procedimiento, resultado esperado y criterio: [PNF-02 en 4.3 del informe](../../../informe-practica-4.md#43-pruebas-no-funcionales). Los valores se anotan primero en la [hoja de registro](../../hojas-registro/hoja-pruebas-no-funcionales.md).

| Archivo esperado | Cómo obtenerlo |
|---|---|
| `../serial-sesion.txt` (log común de la sesión) | Log del monitor serie de toda la sesión: `pio device monitor -f time \| tee docs/evidencias/pruebas/serial-sesion.txt` en Git Bash, iniciado en el bloque 0 (4.1). Se usan las líneas `Distancia:` de las 5 lecturas `DISTANCE` por distancia. |
| `../PF-04/historial-sesion.csv` | Se exporta en PF-04: consola de DynamoDB → `SmartParkHistory` → *Query* por `thingName` y rango de `eventKey` de la sesión → *Download results to CSV*. Se usan los ítems `occupancy_changed`. |

Las rutas con `../` apuntan a archivos compartidos con otras pruebas. Esta carpeta se completa durante la sesión de banco; no contiene resultados todavía.
