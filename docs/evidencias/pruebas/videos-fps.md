# Marcas de tiempo de los videos de la sesión

Resumen generado con [`tools/medir_videos.py`](../../../tools/medir_videos.py) sobre los 23 videos grabados por Isabella Montes en la sesión del 22/11/2026:

```sh
cd docs/evidencias/pruebas
python ../../../tools/medir_videos.py pts --tabla */*.mp4
```

"Variable" significa que al menos un intervalo entre cuadros difiere del resto en más de 2 ms: en estos videos son cuadros sueltos de 66,7 ms (un cuadro descartado por el celular). Por eso los tiempos se calculan con el `pts_time` de cada cuadro (`medir_videos.py delta`) y no con Δcuadros / fps. Los índices de cuadro empiezan en 0.

| Video | fps medio | Cuadros | Duracion (s) | Intervalo min / max (ms) | fps |
|---|---:|---:|---:|---|---|
| `PF-01/pf01-sesion.mp4` | 29.97 | 1357 | 45.238 | 33.2 / 66.7 | variable |
| `PF-02/Abrir-Cerrar.mp4` | 26.78 | 1077 | 40.173 | 33.2 / 66.7 | variable |
| `PF-02/AutoEsperando.mp4` | 30.00 | 213 | 7.067 | 33.3 / 33.4 | constante |
| `PF-02/ConsultarDistancia.mp4` | 30.00 | 362 | 12.035 | 33.3 / 33.4 | constante |
| `PF-02/ConsultarEstado.mp4` | 29.90 | 322 | 10.734 | 33.3 / 66.7 | variable |
| `PF-04/Abrir-CerrarPorTerminal.mp4` | 30.00 | 554 | 18.435 | 33.3 / 33.4 | constante |
| `PF-06/Abrir.mp4` | 29.95 | 566 | 18.867 | 33.3 / 66.7 | variable |
| `PF-06/Abrir1.mp4` | 29.92 | 383 | 12.767 | 33.3 / 66.7 | variable |
| `PF-06/Abrir2.mp4` | 29.94 | 522 | 17.400 | 33.3 / 66.7 | variable |
| `PF-07/AbrirCuandoTodosEspaciosOcupados.mp4` | 29.96 | 773 | 25.767 | 33.3 / 66.7 | variable |
| `PF-07/Antirrebote1001ms.mp4` | 30.00 | 201 | 6.667 | 33.3 / 33.3 | constante |
| `PF-07/Antirrebote120ms.mp4` | 29.90 | 290 | 9.667 | 33.3 / 66.7 | variable |
| `PF-07/Antirrebote120ms-2.mp4` | 29.86 | 217 | 7.233 | 33.3 / 66.7 | variable |
| `PF-07/Antirrebote200ms.mp4` | 29.86 | 381 | 12.698 | 31.7 / 63.3 | variable |
| `PF-07/Ayuda.mp4` | 29.98 | 374 | 12.445 | 31.7 / 63.3 | variable |
| `PF-07/Cancelar.mp4` | 29.82 | 165 | 5.500 | 33.3 / 66.7 | variable |
| `PF-07/ConsultaAntirrebote.mp4` | 30.00 | 235 | 7.800 | 33.3 / 33.3 | constante |
| `PF-07/ConsultaModoParqueo.mp4` | 29.92 | 387 | 12.900 | 33.3 / 66.7 | variable |
| `PF-07/Detener.mp4` | 30.00 | 222 | 7.367 | 33.3 / 33.3 | constante |
| `PF-07/EspaciosDisponibles.mp4` | 30.00 | 149 | 4.934 | 33.3 / 33.4 | constante |
| `PF-07/EspaciosOcupados.mp4` | 24.00 | 162 | 6.707 | 41.7 / 41.8 | constante |
| `PF-07/Plaza2Ocupada-1.mp4` | 29.51 | 45 | 1.483 | 33.3 / 41.7 | variable |
| `PF-07/Plaza2Ocupada-2.mp4` | 24.00 | 200 | 8.290 | 41.7 / 41.8 | constante |
