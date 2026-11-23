# PNF-06 — Reconexión y operación sin conexión

**Requisitos:** RNF6, RNF10, RF13 (E4). Procedimiento y criterio: [4.3 del informe](../../../informe-practica-4.md#43-pruebas-no-funcionales). Análisis: fichas [F3](../../../informe-practica-4.md#f3-reconexión-en-pnf-06) y [F9](../../../informe-practica-4.md#f9-operación-sin-conexión-y-e4-pnf-06).

| Fuente | Qué contiene |
|---|---|
| [`../PNF-01/registro-no-funcionales.md`](../PNF-01/registro-no-funcionales.md#pnf-04-pdf--pnf-06-del-informe--reconexión-wi-fi-y-mqtt-rnf6) | 3 cortes: tiempo hasta Wi-Fi, hasta MQTT y si se aplicó el `desired` pendiente (tabla titulada "PNF-04" en el PDF) |

**Resultado RNF6.** Wi-Fi en 2, 1 y 4 min; MQTT en 1, 3 y 1 min; `desired` pendiente aplicado 3/3. Resolución en minutos y sin la duración de cada corte. Ningún tiempo cumple ≤ 60 s; la sincronización cumple. Causa en el código: el *backoff* de reconexión crece hasta 300 s durante un corte largo.

**Resultado RNF10 y E4.** Sin registro de lo ocurrido en el objeto durante los cortes (no hay log serie). Cubiertos por PA-02: cola del histórico sin red y desborde; E4 en [`test_aws.cpp#L83-L85`](../../../../test/host/test_aws.cpp#L83-L85).

**Estado:** RNF6 ❌ No cumple; RNF10 y E4 Verificada con PA-02.
