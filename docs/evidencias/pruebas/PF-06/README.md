# PF-06 — Lógica Edge y convivencia con la voz

**Requisitos:** RF13 (E1–E3), RF14. Procedimiento y criterio: [4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales). Análisis: [ficha F11](../../../informe-practica-4.md#f11-reglas-del-edge-y-cambio-de-modo-pf-06).

**Procedimiento planificado.** Con las 4 plazas ocupadas en `automatico`, pedir apertura (bloqueada, LCD `APERTURA BLOQ.`) y cierre (aceptado); liberar la plaza 4 y abrir (aceptada); volver a llenar, pasar a `manual` y abrir (aceptada, LCD `PARQUEO LLENO`).

| Archivo | Qué demuestra |
|---|---|
| [`Abrir.mp4`](Abrir.mp4), [`Abrir1.mp4`](Abrir1.mp4), [`Abrir2.mp4`](Abrir2.mp4) | Aperturas de la barrera; el escenario de cada video no está identificado |
| [`../PF-07/AbrirCuandoTodosEspaciosOcupados.mp4`](../PF-07/AbrirCuandoTodosEspaciosOcupados.mp4) | Intento de apertura con todas las plazas ocupadas (E2) |

**Resultado.** E1 y E2 con evidencia parcial; el cierre con el parqueo lleno (E3), la apertura en `manual` con el parqueo lleno y el cambio de modo por voz no tienen un archivo identificable. Cubiertos por PA-02 (E2 en [`test_aws.cpp#L61-L62`](../../../../test/host/test_aws.cpp#L61-L62), E3 en [`test_aws.cpp#L254-L256`](../../../../test/host/test_aws.cpp#L254-L256)) y PA-03 (`test_v2_*`). Los videos no tienen conteo de cuadros, por lo que no sirven para [PNF-04](../PNF-04/).

**Estado:** ⚠️ Cumple parcial (E1, E2); E3 Verificada con PA-02.
