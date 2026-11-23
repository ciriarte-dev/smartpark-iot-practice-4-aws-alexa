# PNF-02 — Exactitud de los sensores

**Requisitos:** RNF2 (HC-SR04) y RNF3 (IR). Procedimiento y criterio: [4.3 del informe](../../../informe-practica-4.md#43-pruebas-no-funcionales). Análisis del IR: [ficha F7](../../../informe-practica-4.md#f7-exactitud-de-los-ir-pnf-02).

| Fuente | Qué contiene |
|---|---|
| [`../PNF-01/registro-no-funcionales.md`](../PNF-01/registro-no-funcionales.md#pnf-02--exactitud-del-hc-sr04-rnf2) | 5 lecturas a 5, 10, 15, 20, 30, 40 y 50 cm |
| [`../PF-01/pf01.md`](../PF-01/pf01.md) | 30 lecturas a 10, 20 y 30 cm |

**Resultado RNF2.** 45 lecturas en las distancias declaradas (10, 20 y 30 cm): error absoluto máximo 1,0 cm (lectura de 21,0 cm a 20 cm, en el PDF) y error medio 0,24 cm. Cumple en el límite de ≤ 1 cm.

**Resultado RNF3.** No hay registro de las 40 transiciones IR.

**Estado:** RNF2 ✅ Cumple; RNF3 No ejecutada (sin registro de las transiciones).
