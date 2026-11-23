# PNF-05 — Salida de `tools/analizar_pruebas.py`

Comando (desde la raíz del repositorio):

```
python tools/analizar_pruebas.py docs/evidencias/pruebas/PNF-05/historial-sesion.csv
```

Entrada: [`historial-sesion.csv`](historial-sesion.csv), 84 eventos del firmware (ambas exportaciones de PF-04 deduplicadas por `eventKey`, sin los 2 ítems `prueba-*`), generado por [`analizar_historial.py`](../../analisis/analizar_historial.py). Sin `--serial` ni `--cloudwatch` porque la sesión no dejó log serie ni exportación de CloudWatch: M/N (RNF7) y Δ1 (RNF8) no se calculan.

Salida literal:

| Indicador | n | Mínimo (s) | Promedio (s) | Máximo (s) | P95 (s) | Criterio | Estado |
|---|---|---|---|---|---|---|---|
| Integridad M/N (RNF7) | 84/? | — | — | — | — | ≥ 99 % | Falta N (--serial o --n) |
| Δ0 = receivedAt − occurredAt (RNF8) | 84 | 0,2 | 42,0 | 179,1 | 171,8 | máx ≤ 15 s | ❌ No cumple |
| Intervalo entre `sample` (RNF5) | 7 | 44,8 | 199,6 | 300,0 | — (n < 20) | mediana 300 ± 10 s | ❌ No cumple |

Ítems en la ventana por tipo: `barrier_changed` 38, `occupancy_changed` 38, `sample` 8
