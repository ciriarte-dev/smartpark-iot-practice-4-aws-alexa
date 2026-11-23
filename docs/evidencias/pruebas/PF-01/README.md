# PF-01 — Lectura de sensores y reporte al Shadow

**Requisitos:** RF1, RF2 (y RNF2 junto con [PNF-02](../PNF-02/)). Procedimiento y criterio: [4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales).

**Procedimiento.** 10 series de `DISTANCE` con un objeto a 10, 20 y 30 cm; `SLOT1` cubriendo y liberando el IR de la plaza 1; revisión de `reported` en el Shadow.

| Archivo | Qué demuestra |
|---|---|
| [`pf01.xlsx`](pf01.xlsx) → [`pf01.md`](pf01.md) | 30 lecturas a 10/20/30 cm con la detección de vehículo |
| [`pf01-sesion.mp4`](pf01-sesion.mp4) | Secuencia de PF-01 grabada el 22/11/2026 |
| [`pf01-10cm.jpeg`](pf01-10cm.jpeg), [`pf01-10cmResultado.jpeg`](pf01-10cmResultado.jpeg) | Objeto a 10 cm y su lectura |
| [`pf01-20cm.jpeg`](pf01-20cm.jpeg), [`pf01-20cmResultado.jpeg`](pf01-20cmResultado.jpeg) | Objeto a 20 cm y su lectura |
| [`pf01-30cm.jpeg`](pf01-30cm.jpeg), [`pf01-30cmResultado.jpeg`](pf01-30cmResultado.jpeg) | Objeto a 30 cm y su lectura |
| [`../../aws-iot/shadow-publica.png`](../../aws-iot/shadow-publica.png) | `reported` con `distancia_cm`, `vehiculo_detectado`, `barrera` y `barrera_posicion_estimada` (08/11/2026) |

La hoja de la sesión anota el cambio de la plaza 1 de libre a ocupada en `update/accepted` a las 19:24:41 (UTC−4) del 22/11.

**Resultado.** 30 lecturas con error máximo de 0,5 cm y error medio de 0,20 cm; detección correcta 30/30. Los atributos de plazas, modo y antirrebote en `reported` se verifican en [PF-07](../PF-07/), porque Alexa los lee de `reported`.

**Estado:** ✅ Cumple (RF1, RF2).
