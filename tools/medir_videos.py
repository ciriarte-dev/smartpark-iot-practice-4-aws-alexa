#!/usr/bin/env python3
"""Medicion de tiempos en los videos de la sesion por marca de tiempo de cada cuadro.

Los videos de celular y de WhatsApp pueden tener fps variable, asi que los tiempos
se calculan con el `pts_time` de cada cuadro (ffprobe) y no con Δcuadros / fps.
Los indices de cuadro empiezan en 0 y siguen el orden de presentacion.

Subcomandos:
  pts <video> [<video> ...] [--tabla]
        Indice y pts_time de cada cuadro y resumen (fps medio, n.o de cuadros,
        duracion, fps constante o variable). Con --tabla solo imprime una fila
        Markdown de resumen por video.
  hoja <video> --desde S --hasta S --salida X.jpg [--rotar 90|180|270]
        [--recorte W:H:X:Y] [--columnas N] [--ancho PX]
        Hoja de contacto con todos los cuadros del intervalo, cada uno rotulado con
        su indice y su pts_time absoluto (no se busca con -ss antes de -i, que
        reiniciaria el pts). El recorte se aplica despues de la rotacion.
  delta <video> --cuadro0 N --cuadro1 M
        Δt = pts(M) - pts(N) en segundos, con la incertidumbre de ±1 cuadro
        (duracion media de un cuadro del video).

Requiere ffprobe y ffmpeg en el PATH. Solo usa la biblioteca estandar de
Python 3.9 o superior.
"""
import argparse
import os
import statistics
import subprocess
import sys

TOLERANCIA_VFR_S = 0.002   # Diferencia entre intervalos que se considera fps variable
FUENTE = "C\\:/Windows/Fonts/arial.ttf" if os.name == "nt" else "DejaVuSans.ttf"


def ffprobe(argumentos):
    """Ejecuta ffprobe y devuelve su salida estandar como texto."""
    salida = subprocess.run(["ffprobe", "-v", "error", *argumentos],
                            capture_output=True, text=True, check=True)
    return salida.stdout


def cuadros(video):
    """Lista ordenada de pts_time (s) de los cuadros de video, en orden de presentacion."""
    texto = ffprobe(["-select_streams", "v:0", "-show_entries",
                     "frame=best_effort_timestamp_time", "-of", "default=nw=1:nk=1", video])
    tiempos = [float(linea.strip())
               for linea in texto.splitlines() if linea.strip() not in ("", "N/A")]
    return sorted(tiempos)


def resumen(video, tiempos):
    """Fps medio, duracion e indicador de fps constante a partir de los pts."""
    texto = ffprobe(["-select_streams", "v:0", "-show_entries",
                     "stream=avg_frame_rate", "-of", "default=nw=1:nk=1", video]).split()[0]
    num, _, den = texto.partition("/")
    fps_medio = float(num) / float(den or 1)
    intervalos = [b - a for a, b in zip(tiempos, tiempos[1:])]
    variable = (max(intervalos) - min(intervalos)) > TOLERANCIA_VFR_S if intervalos else False
    return {
        "fps_medio": fps_medio,
        "cuadros": len(tiempos),
        "duracion": tiempos[-1] - tiempos[0] if tiempos else 0.0,
        "variable": variable,
        "intervalo_min": min(intervalos) if intervalos else 0.0,
        "intervalo_max": max(intervalos) if intervalos else 0.0,
        "intervalo_medio": statistics.mean(intervalos) if intervalos else 0.0,
    }


def comando_pts(args):
    """Imprime los pts de cada cuadro o una tabla de resumen."""
    if args.tabla:
        print("| Video | fps medio | Cuadros | Duracion (s) | Intervalo min / max (ms) | fps |")
        print("|---|---:|---:|---:|---|---|")
    for video in args.videos:
        tiempos = cuadros(video)
        datos = resumen(video, tiempos)
        if args.tabla:
            tipo = "variable" if datos["variable"] else "constante"
            print(f"| `{video}` | {datos['fps_medio']:.2f} | {datos['cuadros']} | "
                  f"{datos['duracion']:.3f} | {datos['intervalo_min'] * 1000:.1f} / "
                  f"{datos['intervalo_max'] * 1000:.1f} | {tipo} |")
            continue
        print(f"# {video}: {datos['cuadros']} cuadros, fps medio {datos['fps_medio']:.3f}, "
              f"{'variable' if datos['variable'] else 'constante'}")
        print("cuadro,pts_time")
        for indice, tiempo in enumerate(tiempos):
            print(f"{indice},{tiempo:.6f}")


def comando_hoja(args):
    """Genera la hoja de contacto rotulada con indice y pts absoluto."""
    tiempos = cuadros(args.video)
    seleccion = [i for i, t in enumerate(tiempos) if args.desde - 1e-6 <= t <= args.hasta + 1e-6]
    if not seleccion:
        sys.exit("No hay cuadros en el intervalo indicado.")
    primero = seleccion[0]
    filas = -(-len(seleccion) // args.columnas)
    columnas = min(args.columnas, len(seleccion))
    # El separador de 2 px entre celdas cuenta dentro del ancho total; scale exige ancho par.
    ancho_celda = (args.ancho - 2 * (columnas - 1)) // columnas // 2 * 2
    filtros = [f"select='between(t\\,{args.desde - 1e-6}\\,{args.hasta + 1e-6})'"]
    filtros += {90: ["transpose=1"], 180: ["hflip", "vflip"], 270: ["transpose=2"]}.get(args.rotar, [])
    if args.recorte:
        filtros.append(f"crop={args.recorte}")
    filtros.append(f"scale={ancho_celda}:-2")
    alto_texto = max(12, ancho_celda // 12)
    filtros.append(
        f"drawtext=fontfile='{FUENTE}':fontsize={alto_texto}:fontcolor=yellow:box=1:"
        f"boxcolor=black@0.7:x=4:y=4:text='#%{{eif\\:n+{primero}\\:d}}  %{{pts\\:flt}} s'")
    filtros.append(f"tile={columnas}x{filas}:padding=2:color=white")
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", args.video, "-vf", ",".join(filtros),
                    "-fps_mode", "passthrough", "-frames:v", "1", "-q:v", str(args.calidad),
                    args.salida], check=True)
    print(f"{args.salida}: cuadros {primero}-{seleccion[-1]} "
          f"({tiempos[primero]:.3f}-{tiempos[seleccion[-1]]:.3f} s), {len(seleccion)} cuadros")


def comando_delta(args):
    """Calcula el tiempo entre dos cuadros con su incertidumbre."""
    tiempos = cuadros(args.video)
    for indice in (args.cuadro0, args.cuadro1):
        if not 0 <= indice < len(tiempos):
            sys.exit(f"Cuadro {indice} fuera de rango (0-{len(tiempos) - 1}).")
    datos = resumen(args.video, tiempos)
    delta = tiempos[args.cuadro1] - tiempos[args.cuadro0]
    print(f"pts0={tiempos[args.cuadro0]:.3f} pts1={tiempos[args.cuadro1]:.3f} "
          f"delta={delta:.3f} s ± {datos['intervalo_medio']:.3f} s (±1 cuadro)")


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="comando", required=True)

    p_pts = sub.add_parser("pts", help="pts de cada cuadro y fps del video")
    p_pts.add_argument("videos", nargs="+")
    p_pts.add_argument("--tabla", action="store_true", help="solo una fila de resumen por video")
    p_pts.set_defaults(funcion=comando_pts)

    p_hoja = sub.add_parser("hoja", help="hoja de contacto de un intervalo")
    p_hoja.add_argument("video")
    p_hoja.add_argument("--desde", type=float, required=True)
    p_hoja.add_argument("--hasta", type=float, required=True)
    p_hoja.add_argument("--salida", required=True)
    p_hoja.add_argument("--rotar", type=int, choices=(90, 180, 270))
    p_hoja.add_argument("--recorte", help="W:H:X:Y en pixeles, despues de rotar")
    p_hoja.add_argument("--columnas", type=int, default=6)
    p_hoja.add_argument("--ancho", type=int, default=1280, help="ancho total de la hoja")
    p_hoja.add_argument("--calidad", type=int, default=4, help="calidad JPG de ffmpeg (2-31)")
    p_hoja.set_defaults(funcion=comando_hoja)

    p_delta = sub.add_parser("delta", help="tiempo entre dos cuadros")
    p_delta.add_argument("video")
    p_delta.add_argument("--cuadro0", type=int, required=True)
    p_delta.add_argument("--cuadro1", type=int, required=True)
    p_delta.set_defaults(funcion=comando_delta)

    args = parser.parse_args()
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    args.funcion(args)


if __name__ == "__main__":
    main()
