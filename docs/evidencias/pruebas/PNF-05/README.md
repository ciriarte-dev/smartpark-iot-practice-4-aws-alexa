# PNF-05 — Almacenamiento y frecuencia de reporte

**Requisitos:** RNF5, RNF7, RNF8. Procedimiento y criterio: [4.3 del informe](../../../informe-practica-4.md#43-pruebas-no-funcionales). Análisis: fichas [F4](../../../informe-practica-4.md#f4-latencia-del-histórico-en-ráfagas-pnf-05), [F5](../../../informe-practica-4.md#f5-frecuencia-de-las-muestras-pnf-05) y [F6](../../../informe-practica-4.md#f6-integridad-del-almacenamiento-pf-05-y-pnf-05).

| Archivo | Qué contiene |
|---|---|
| [`historial-sesion.csv`](historial-sesion.csv) | 84 eventos del firmware: las dos exportaciones de [PF-04](../PF-04/) deduplicadas por `eventKey`, sin los 2 ítems `prueba-*` |
| [`analisis.md`](analisis.md) | Salida de `tools/analizar_pruebas.py` sobre ese CSV |
| [`analisis.txt`](analisis.txt) | Uso completo de `tools/analizar_pruebas.py` con log serie y CloudWatch |
| [`../../analisis/historial-resultados.md`](../../analisis/historial-resultados.md) | Δ0 por grupo, intervalos entre muestras e integridad por número de secuencia ([`analizar_historial.py`](../../analisis/analizar_historial.py)) |

**Resultados.**

| RNF | Medido | Criterio | Estado |
|---|---|---|---|
| RNF5 | Intervalos entre `sample`: 272,7; 109,3; 207,4; 44,8; 162,7; 300,0; 300,0 s. Los 5 cortos empiezan un arranque nuevo; dentro de un arranque, 300,0 s (2/2). Respaldo del Shadow sin log serie | 300 ± 10 s; 30 ± 1 s | ⚠️ Cumple parcial |
| RNF7 | M/N = 48/61 (78,7 %) reconstruido con el número de secuencia; ráfaga del 19/11: 38/50 por llenado de la cola de 16 eventos. Inválidos y duplicados con PA-03 | ≥ 99 %; 100 % | ❌ No cumple |
| RNF8 | Δ1 sin CloudWatch. Δ0 (84 eventos): mín 0,2 s, mediana 9,7 s, promedio 42,0 s, máx 179,1 s, P95 171,8 s; aislados 37/37 ≤ 15 s (máx 9,8 s), en cola 10/47 | Δ1 ≤ 5 s; Δ0 ≤ 15 s | ❌ No cumple |
