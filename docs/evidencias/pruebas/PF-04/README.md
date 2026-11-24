# PF-04 — Almacenamiento del histórico

**Requisitos:** RF10, RF11, RF12. Procedimiento y criterio: [4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales). Datos: [5.3 del informe](../../../informe-practica-4.md#53-datos-almacenados-en-dynamodb).

**Procedimiento.** Órdenes `OPEN` y `CLOSE` por el monitor serie y órdenes por voz; exportación de `SmartParkHistory` desde la consola de DynamoDB.

| Archivo | Qué demuestra |
|---|---|
| [`Abrir-CerrarPorTerminal.mp4`](Abrir-CerrarPorTerminal.mp4) | `OPEN` y `CLOSE` por Serial y movimiento de la barrera (origen `edge`) |
| [`historial-barrera.csv`](historial-barrera.csv) | 38 `barrier_changed`: 31 `legado` (órdenes V1, incluida la voz) y 7 `edge` |
| [`historial-muestra.csv`](historial-muestra.csv) | Página de 50 filas: 8 `sample`, 40 `occupancy_changed` y 2 `barrier_changed` repetidos del otro CSV |

Análisis reproducible: [`../../analisis/historial-resultados.md`](../../analisis/historial-resultados.md).

**Resultado.** 88 filas, 86 ítems únicos por `eventKey`; se excluyen 2 ítems `prueba-*` publicados a mano desde el cliente MQTT; quedan 84 eventos del firmware con los 9 atributos del modelo. La skill envía las órdenes en formato V1 (`desired.barrera`), por lo que la voz figura con `origin = legado`; el origen `voz` solo lo produce el formato V2, que la Lambda desplegada no usa. Las lecturas son del 19 y 20/11 con `capacidad = 1` (desarrollo); la sesión del 22/11 solo se exportó para la barrera (27 `legado`, 4 `edge`).

**Estado:** ✅ Cumple (RF10, RF12); ⚠️ Cumple parcial (RF11, [ficha F12](../../../informe-practica-4.md#f12-lecturas-almacenadas-pf-04-rf11)).
