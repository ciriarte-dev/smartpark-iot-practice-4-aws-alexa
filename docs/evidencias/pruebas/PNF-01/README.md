# PNF-01 — Estabilidad

**Requisito:** RNF1. Procedimiento y criterio: [4.3 del informe](../../../informe-practica-4.md#43-pruebas-no-funcionales). Análisis: [ficha F1](../../../informe-practica-4.md#f1-reinicios-en-pnf-01).

Esta carpeta guarda la hoja de mediciones no funcionales de la sesión del 22/11/2026, que también usan [PNF-02](../PNF-02/), [PNF-03](../PNF-03/) y [PNF-06](../PNF-06/).

| Archivo | Qué contiene |
|---|---|
| [`registro-no-funcionales.pdf`](registro-no-funcionales.pdf) | Hoja original: estabilidad, HC-SR04, tiempos de Alexa y reconexión |
| [`registro-no-funcionales.md`](registro-no-funcionales.md) | Transcripción literal, con estadísticas calculadas y notas de transcripción |

La tabla del PDF titulada "PNF-04" corresponde a la reconexión (PNF-06 del informe).

**Resultado.** 10 min de operación, 3 reinicios, 0 bloqueos, 1 desconexión y 10 UPDATE aceptados, frente a ≥ 15 min con 0/0/0. Causa probable: caída de tensión al mover el servo alimentado desde el pin 3V3 del ESP32; no se confirmó porque el firmware no imprime la causa del reinicio.

**Estado:** ❌ No cumple.
