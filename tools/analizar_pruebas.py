#!/usr/bin/env python3
"""Indicadores de PNF-05 (RNF5, RNF7 y RNF8) a partir de las exportaciones de la sesion.

Entradas:
  historial.csv   Exportacion CSV de la tabla SmartParkHistory (consola de DynamoDB).
  --serial        Log del monitor serie generado con `pio device monitor -f time`.
  --cloudwatch    Exportacion CSV de CloudWatch Logs Insights (@timestamp, @message)
                  del grupo /aws/lambda/SmartParkHistoryProcessor.
  --inicio/--fin  Ventana de PNF-05 en ISO 8601 con zona (2026-11-24T10:00:00-04:00).
  --n             Eventos publicados en la ventana, si no se toma del log serie.

Salida: tabla Markdown para la seccion 5.2 del informe.
Solo usa la biblioteca estandar de Python 3.9 o superior.
"""
import argparse
import codecs
import csv
import math
import re
import statistics
import sys
from datetime import datetime, timezone

INTEGRIDAD_MIN = 99.0          # RNF7: porcentaje minimo
DELTA1_MAX_S = 5.0             # RNF8: REPORT - receivedAt
DELTA0_MAX_S = 15.0            # RNF8: receivedAt - occurredAt
MUESTRA_S, MUESTRA_TOL_S = 300.0, 10.0   # RNF5: muestra del historico
RESPALDO_S, RESPALDO_TOL_S = 30.0, 1.0   # RNF5: respaldo periodico del Shadow
MARGEN_VENTANA_MS = 5000       # Eventos publicados al final de la ventana

LINEA_SERIAL = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) > (.*)$")
HISTORICO = re.compile(r"Historico: MQTT local=(\d+)")
UPDATE = re.compile(r"AWS UPDATE clase=(\d+)")


def columna(nombre):
    """Normaliza encabezados como `"receivedAt (N)"` a `receivedAt`."""
    return re.sub(r"\s*\([A-Z]+\)$", "", nombre.strip().strip('"'))


def leer_historial(ruta):
    with open(ruta, encoding="utf-8-sig", newline="") as archivo:
        lineas = [l for l in archivo if l.strip() and not l.lstrip().startswith("#")]
    filas = []
    for fila in csv.DictReader(lineas):
        item = {columna(k): (v or "").strip() for k, v in fila.items() if k}
        try:
            item["occurredAt"] = int(float(item["occurredAt"]))
            item["receivedAt"] = int(float(item["receivedAt"]))
        except (KeyError, ValueError):
            sys.exit(f"Fila sin occurredAt/receivedAt numericos: {fila}")
        filas.append(item)
    if not filas:
        sys.exit("El CSV no tiene filas.")
    return filas


def epoch_ms(texto):
    instante = datetime.fromisoformat(texto)
    if instante.tzinfo is None:
        sys.exit(f"Indique la zona horaria en {texto} (por ejemplo -04:00).")
    return int(instante.timestamp() * 1000)


def segundos_del_dia(texto):
    """Hora de pared (PC) de una fecha ISO, comparable con el log de `-f time`."""
    instante = datetime.fromisoformat(texto)
    return instante.hour * 3600 + instante.minute * 60 + instante.second


def leer_serial(ruta):
    with open(ruta, "rb") as archivo:
        crudo = archivo.read()
    # `tee` de Windows PowerShell 5.1 escribe UTF-16; Git Bash y Linux, UTF-8.
    utf16 = crudo.startswith(codecs.BOM_UTF16_LE) or crudo.startswith(codecs.BOM_UTF16_BE)
    texto = crudo.decode("utf-16") if utf16 else crudo.decode("utf-8", "replace")
    eventos, previo, dia = [], None, 0
    for linea in texto.splitlines():
        m = LINEA_SERIAL.match(linea)
        if not m:
            continue
        t = int(m[1]) * 3600 + int(m[2]) * 60 + int(m[3]) + int(m[4]) / 1000 + dia
        if previo is not None and t < previo - 43200:
            dia += 86400
            t += 86400
        previo = t
        eventos.append((t, m[5]))
    return eventos


def publicados_en_ventana(serial, desde_s, hasta_s):
    contadores = [(t, int(m[1])) for t, texto in serial
                  if (m := HISTORICO.search(texto))
                  and (desde_s is None or desde_s - 60 <= t % 86400 <= hasta_s + 60)]
    if len(contadores) < 2:
        sys.exit("El log serie necesita dos lineas `Historico: MQTT local=` (STATUS al inicio y al fin de la ventana).")
    return contadores[-1][1] - contadores[0][1]


def respaldos_shadow(serial):
    """Intervalos entre UPDATE consecutivos de clase 0 (Periodic) sin otra clase en medio."""
    updates = [(t, int(m[1])) for t, texto in serial if (m := UPDATE.search(texto))]
    return [b[0] - a[0] for a, b in zip(updates, updates[1:]) if a[1] == 0 and b[1] == 0]


def leer_cloudwatch(ruta):
    reportes = []
    with open(ruta, encoding="utf-8-sig", newline="") as archivo:
        for fila in csv.DictReader(archivo):
            mensaje = fila.get("@message", "")
            if mensaje.startswith("REPORT"):
                instante = datetime.strptime(fila["@timestamp"][:23], "%Y-%m-%d %H:%M:%S.%f")
                reportes.append(int(instante.replace(tzinfo=timezone.utc).timestamp() * 1000))
    return sorted(reportes)


def emparejar(items, reportes):
    """Asigna a cada item el primer REPORT libre posterior a su receivedAt."""
    deltas, libres = [], list(reportes)
    for item in sorted(items, key=lambda i: i["receivedAt"]):
        candidato = next((r for r in libres if r >= item["receivedAt"]), None)
        if candidato is not None:
            libres.remove(candidato)
            deltas.append((candidato - item["receivedAt"]) / 1000)
    return deltas


def num(valor):
    """Un decimal con coma, como en el informe."""
    return f"{valor:.1f}".replace(".", ",")


def p95(valores):
    if len(valores) < 20:
        return "— (n < 20)"
    return num(sorted(valores)[math.ceil(0.95 * len(valores)) - 1])


def estado(cumple):
    return "✅ Cumple" if cumple else "❌ No cumple"


def fila_tiempos(nombre, valores, criterio, cumple):
    if not valores:
        return f"| {nombre} | 0 | — | — | — | — | {criterio} | Sin datos |"
    return (f"| {nombre} | {len(valores)} | {num(min(valores))} | {num(statistics.mean(valores))} | "
            f"{num(max(valores))} | {p95(valores)} | {criterio} | {estado(cumple(valores))} |")


def main():
    # La tabla usa ≥, Δ y emojis: UTF-8 tambien en consolas y redirecciones de Windows.
    sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("historial")
    parser.add_argument("--serial")
    parser.add_argument("--cloudwatch")
    parser.add_argument("--inicio")
    parser.add_argument("--fin")
    parser.add_argument("--n", type=int)
    args = parser.parse_args()
    if bool(args.inicio) != bool(args.fin):
        sys.exit("Use --inicio y --fin juntos.")

    items = leer_historial(args.historial)
    serial = leer_serial(args.serial) if args.serial else []
    if args.inicio:
        desde, hasta = epoch_ms(args.inicio), epoch_ms(args.fin)
        ventana = [i for i in items if desde <= i["receivedAt"] <= hasta + MARGEN_VENTANA_MS]
        desde_s, hasta_s = segundos_del_dia(args.inicio), segundos_del_dia(args.fin)
    else:
        ventana, desde_s, hasta_s = items, None, None

    filas = ["| Indicador | n | Mínimo (s) | Promedio (s) | Máximo (s) | P95 (s) | Criterio | Estado |",
             "|---|---|---|---|---|---|---|---|"]
    n = args.n if args.n is not None else (publicados_en_ventana(serial, desde_s, hasta_s) if serial else None)
    m = len(ventana)
    if n:
        porcentaje = 100 * m / n
        resultado = "Revisar límites de la ventana (M > N)" if m > n else estado(porcentaje >= INTEGRIDAD_MIN)
        filas.append(f"| Integridad M/N (RNF7) | {m}/{n} | — | {num(porcentaje)} % | — | — | "
                     f"≥ {INTEGRIDAD_MIN:.0f} % | {resultado} |")
    else:
        filas.append(f"| Integridad M/N (RNF7) | {m}/? | — | — | — | — | ≥ {INTEGRIDAD_MIN:.0f} % | Falta N (--serial o --n) |")

    delta0 = [(i["receivedAt"] - i["occurredAt"]) / 1000 for i in ventana]
    filas.append(fila_tiempos("Δ0 = receivedAt − occurredAt (RNF8)", delta0,
                              f"máx ≤ {DELTA0_MAX_S:.0f} s", lambda v: max(v) <= DELTA0_MAX_S))
    if args.cloudwatch:
        delta1 = emparejar(ventana, leer_cloudwatch(args.cloudwatch))
        filas.append(fila_tiempos("Δ1 = REPORT − receivedAt (RNF8)", delta1,
                                  f"máx ≤ {DELTA1_MAX_S:.0f} s", lambda v: max(v) <= DELTA1_MAX_S))

    muestras = sorted(i["occurredAt"] for i in items if i.get("eventType") == "sample")
    intervalos = [(b - a) / 1000 for a, b in zip(muestras, muestras[1:])]
    filas.append(fila_tiempos("Intervalo entre `sample` (RNF5)", intervalos,
                              f"mediana {MUESTRA_S:.0f} ± {MUESTRA_TOL_S:.0f} s",
                              lambda v: abs(statistics.median(v) - MUESTRA_S) <= MUESTRA_TOL_S))
    if serial:
        filas.append(fila_tiempos("Respaldo del Shadow, `AWS UPDATE clase=0` (RNF5)", respaldos_shadow(serial),
                                  f"mediana {RESPALDO_S:.0f} ± {RESPALDO_TOL_S:.0f} s",
                                  lambda v: abs(statistics.median(v) - RESPALDO_S) <= RESPALDO_TOL_S))

    tipos = {}
    for item in ventana:
        tipos[item.get("eventType", "?")] = tipos.get(item.get("eventType", "?"), 0) + 1
    print("\n".join(filas))
    print()
    print("Ítems en la ventana por tipo: " + ", ".join(f"`{k}` {v}" for k, v in sorted(tipos.items())))


if __name__ == "__main__":
    main()
