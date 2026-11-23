# PF-07 — Consultas y cambios de modo y antirrebote por voz

**Requisitos:** RF14, RF3. Procedimiento y criterio: [4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales).

**Procedimiento.** Consultar plazas disponibles y ocupadas, la plaza 2, el modo y el antirrebote; ajustar el antirrebote a 200 y 120 ms y a 1001 ms (fuera de rango); ayuda, cancelar y detener.

| Archivo | Intent o escenario |
|---|---|
| [`EspaciosDisponibles.mp4`](EspaciosDisponibles.mp4) | `ConsultarDisponiblesIntent` |
| [`EspaciosOcupados.mp4`](EspaciosOcupados.mp4) | `ConsultarOcupadosIntent` |
| [`Plaza2Ocupada-1.mp4`](Plaza2Ocupada-1.mp4), [`Plaza2Ocupada-2.mp4`](Plaza2Ocupada-2.mp4) | `ConsultarEstacionamientoIntent` (plaza 2) |
| [`ConsultaModoParqueo.mp4`](ConsultaModoParqueo.mp4) | `ConsultarModoIntent` |
| [`ConsultaAntirrebote.mp4`](ConsultaAntirrebote.mp4) | `ConsultarAntirreboteIntent` |
| [`Antirrebote200ms.mp4`](Antirrebote200ms.mp4) | `CambiarAntirreboteIntent` a 200 ms |
| [`Antirrebote120ms.mp4`](Antirrebote120ms.mp4), [`Antirrebote120ms-2.mp4`](Antirrebote120ms-2.mp4) | `CambiarAntirreboteIntent` a 120 ms (dos ejecuciones; el segundo se llamaba `Antirrebote120mss.mp4`) |
| [`Antirrebote1001ms.mp4`](Antirrebote1001ms.mp4) | `CambiarAntirreboteIntent` fuera de rango (1001 ms), rechazado |
| [`Ayuda.mp4`](Ayuda.mp4), [`Cancelar.mp4`](Cancelar.mp4), [`Detener.mp4`](Detener.mp4) | `AMAZON.HelpIntent`, `AMAZON.CancelIntent`, `AMAZON.StopIntent` |
| [`AbrirCuandoTodosEspaciosOcupados.mp4`](AbrirCuandoTodosEspaciosOcupados.mp4) | Intento de apertura con todas las plazas ocupadas (E2, ver [PF-06](../PF-06/)) |

**Resultado.** 14 videos, uno por intent o escenario. Las consultas de plazas, modo y antirrebote solo se responden con datos de `reported` de menos de 180 s, por lo que también verifican que el ESP32 reporta esos atributos (RF2).

**Estado:** ✅ Cumple (consultas y ajuste del antirrebote). El cambio de modo por voz no tiene video (RF14 ⚠️ en el informe, [ficha F11](../../../informe-practica-4.md#f11-reglas-del-edge-y-cambio-de-modo-pf-06)).
