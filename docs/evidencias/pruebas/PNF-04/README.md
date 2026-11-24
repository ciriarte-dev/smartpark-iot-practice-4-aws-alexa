# PNF-04 — Respuesta de la lógica automática en el Edge

**Requisito:** RNF9 (Edge): ≤ 0,3 s desde que el vehículo cubre el IR hasta el cambio de LED. Procedimiento y criterio: [4.3 del informe](../../../informe-practica-4.md#43-pruebas-no-funcionales).

**Fuente.** [`../PF-01/pf01-sesion.mp4`](../PF-01/pf01-sesion.mp4), grabado por Isabella Montes en la sesión del 22/11/2026: un dedo cubre y libera los IR con los LED de las plazas en el mismo cuadro. 1357 cuadros, 29,97 fps de media ([`../videos-fps.md`](../videos-fps.md)). 29,97 fps es la frecuencia estándar nominal de 30 fps, y como los tiempos se calculan con el `pts_time` de cada cuadro, los cuadros sueltos de 66,7 ms no alteran Δt.

**Método (fijado antes de medir).**

- **t0:** el LED indicador del módulo IR no se distingue en el video (848×478 px). Por eso t0 es el primer cuadro en que el dedo tapa el emisor y el receptor del módulo, que es el instante "el vehículo cubre el IR" de RNF9. En la liberación, el primer cuadro con los dos descubiertos.
- **t1:** primer cuadro en que el LED de la plaza cambia de color.
- **Δt** = pts(t1) − pts(t0), con el `pts_time` de cada cuadro: `python ../../../tools/medir_videos.py delta ../PF-01/pf01-sesion.mp4 --cuadro0 N --cuadro1 M`. Incertidumbre ±1 cuadro (±0,033 s).
- **Validez:** se descarta una repetición si el movimiento de cámara o el encuadre no permiten fijar t0 o t1.

**Resultados** ([`mediciones.csv`](mediciones.csv); una hoja de contacto por repetición en [`cuadros/`](cuadros/), rotulada con el índice y el `pts_time` de cada cuadro).

| Rep. | Plaza | Tipo | t0 (cuadro / s) | t1 (cuadro / s) | Δt (s) | Hoja |
|---|---:|---|---|---|---:|---|
| O1 | 1 | Ocupación | 146 / 4,867 | 149 / 4,967 | 0,100 | [`o1`](cuadros/o1-plaza1-ocupacion.jpg) |
| O2 | 2 | Ocupación | 213 / 7,101 | 216 / 7,201 | 0,100 | [`o2`](cuadros/o2-plaza2-ocupacion.jpg) |
| O3 | 4 | Ocupación | 750 / 25,003 | 755 / 25,169 | 0,167 | [`o3`](cuadros/o3-plaza4-ocupacion.jpg) |
| O4 | 1 | Ocupación | 1180 / 39,337 | 1185 / 39,504 | 0,167 | [`o4`](cuadros/o4-plaza1-ocupacion.jpg) |
| L1 | 1 | Liberación | 173 / 5,767 | 176 / 5,867 | 0,100 | [`l1`](cuadros/l1-plaza1-liberacion.jpg) |

n = 4 ocupaciones (5 planificadas): mín 0,100 s, mediana 0,133 s, máx 0,167 s (0,200 s con +1 cuadro). L1 es una liberación y queda como dato complementario, fuera de n. Las liberaciones de las plazas 2, 3 y 4 ocurren fuera de cuadro.

**Descartadas (3).** X1 (plaza 2, 39,87 s): el LED cambia con el receptor todavía visible, porque el IR detecta el dedo junto al módulo; t0 no existe con el criterio (desde el primer contacto serían 0,067 s). X2 (plaza 3, 15,10 s): cámara en movimiento; revisada de nuevo, el desenfoque no deja distinguir el emisor y el receptor entre los cuadros 448 y 451, así que t0 no se puede fijar (Δt quedaría entre 0,067 y 0,167 s). X3 (0,0–0,6 s): la cámara se aparta antes del cambio.

**Análisis.** El diseño fija un mínimo de 120 ms entre la detección y el LED (antirrebote por defecto, [`config.example.h#L73`](../../../../include/config.example.h#L73)) más hasta 10 ms de muestreo. Los Δt de 0,100 s están por debajo de ese mínimo: el IR reflectivo empieza a detectar el dedo antes de que lo tape del todo, como muestra X1. Δt mide entonces el indicador declarado en RNF9 (cubrir → LED), no el retardo interno del Edge, que con este encuadre no se puede aislar. En O2, aun contando desde el primer cuadro con el emisor tapado (207), Δt = 0,300 s, en el límite del criterio.

**Estado:** ✅ Cumple: máx 0,167 s en las 4 ocupaciones medibles (5 planificadas); 1 liberación complementaria de 0,100 s; con el criterio de t0 más conservador, 0,300 s, en el límite de 0,3 s.
