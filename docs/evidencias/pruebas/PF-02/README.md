# PF-02 — Barrera y consultas por voz

**Requisitos:** RF3, RF4, RF5, RF6. Procedimiento y criterio: [4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales).

**Procedimiento.** Abrir y cerrar la barrera por voz 3 veces contrastando `desired.barrera`; consultar distancia, vehículo esperando y estado de la barrera; detección de vehículo a 10, 16, 17 y 18 cm en 6 series.

| Archivo | Qué demuestra |
|---|---|
| [`Abrir-Cerrar.mp4`](Abrir-Cerrar.mp4) | Órdenes de voz de apertura y cierre (`AbrirBarreraIntent`, `CerrarBarreraIntent`) |
| [`ConsultarDistancia.mp4`](ConsultarDistancia.mp4) | Consulta de la distancia (`ConsultarDistanciaIntent`) |
| [`AutoEsperando.mp4`](AutoEsperando.mp4) | Consulta de vehículo esperando (`ConsultarVehiculoIntent`) |
| [`ConsultarEstado.mp4`](ConsultarEstado.mp4) | Consulta del estado de la barrera (`EstadoBarreraIntent`) |
| [`pf02.xlsx`](pf02.xlsx) → [`pf02.md`](pf02.md) | Detección por distancia en 6 series |
| [`pf02-voz-desired.xlsx`](pf02-voz-desired.xlsx) → [`pf02.md`](pf02.md#pf02-voz-desiredxlsx--orden-por-voz-contra-desired) | Orden de voz contra `desired.barrera` en 3 pruebas (antes `PF04 - pf05.xlsx`) |

**Resultado.** `desired.barrera` correcto en 6/6 órdenes. Detección por distancia: 46/48 coincidencias; las 2 diferencias son "No" a 17 cm, dentro de la banda de histéresis de 15–18 cm, donde el firmware conserva el estado anterior ([`UltrasonicSensor.cpp#L29-L37`](../../../../src/UltrasonicSensor.cpp#L29-L37)); en 10 y 18 cm, 24/24. Análisis en [`pf02.md`](pf02.md).

**Estado:** ✅ Cumple (RF3–RF6).
