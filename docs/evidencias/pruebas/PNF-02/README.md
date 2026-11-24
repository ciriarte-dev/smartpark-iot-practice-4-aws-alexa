# PNF-02 — Exactitud de los sensores

**Requisitos:** RNF2 (HC-SR04) y RNF3 (IR). Procedimiento y criterio: [4.3 del informe](../../../informe-practica-4.md#43-pruebas-no-funcionales). Análisis del IR: [ficha F7](../../../informe-practica-4.md#f7-exactitud-de-los-ir-pnf-02).

| Fuente | Qué contiene |
|---|---|
| [`../PNF-01/registro-no-funcionales.md`](../PNF-01/registro-no-funcionales.md#pnf-02--exactitud-del-hc-sr04-rnf2) | 5 lecturas a 5, 10, 15, 20, 30, 40 y 50 cm |
| [`../PF-01/pf01.md`](../PF-01/pf01.md) | 30 lecturas a 10, 20 y 30 cm |
| [`transiciones-ir.csv`](transiciones-ir.csv) | Transiciones IR del video [`pf01-sesion.mp4`](../PF-01/pf01-sesion.mp4) con el cuadro del cambio de LED |

**Resultado RNF2.** 45 lecturas en las distancias declaradas (10, 20 y 30 cm): error absoluto máximo 1,0 cm (lectura de 21,0 cm a 20 cm, en el PDF) y error medio 0,24 cm. Cumple en el límite de ≤ 1 cm.

**Resultado RNF3 (muestra parcial).** En el video de la sesión del 22/11/2026, grabado por Isabella Montes, se ven 7 transiciones IR con el LED en cuadro: 6 ocupaciones (plazas 1, 2, 3, 4, 1 y 2) y 1 liberación (plaza 1). En las 7 cambia el LED de la plaza cubierta y ningún otro: **7/7 aciertos**. El criterio pide 40 transiciones (5 entradas y 5 salidas por plaza); la muestra no cubre las salidas de las plazas 2, 3 y 4.

**Estado:** RNF2 ✅ Cumple; RNF3 ⚠️ Cumple parcial (7/7 en una muestra de 7 de las 40 transiciones).
