#!/usr/bin/env python3
"""Analisis de las exportaciones de SmartParkHistory (PF-04) para PNF-05.

Uso, desde la raiz del repositorio (Python 3.9+, solo biblioteca estandar):
  python docs/evidencias/analisis/analizar_historial.py > docs/evidencias/analisis/historial-resultados.md
En Windows se ejecuta con PYTHONUTF8=1 para que la salida use UTF-8.

Pasos:
  1. Une historial-barrera.csv e historial-muestra.csv y deduplica por eventKey.
  2. Excluye los items cuyo eventId empieza por "prueba" (publicados a mano desde el
     cliente de prueba MQTT, con occurredAt fijado a mano; no los genero el firmware).
  3. Escribe el CSV derivado docs/evidencias/pruebas/PNF-05/historial-sesion.csv.
  4. Calcula d0 = receivedAt - occurredAt y separa dos grupos segun la regla del
     publicador (HISTORY_PUBLISH_INTERVAL_MS = 10000 en include/config.example.h):
       - aislado: el evento ocurrio con el intervalo de 10 s ya libre
         (occurredAt >= receivedAt del evento anterior del mismo arranque + 10 s);
       - en cola: ocurrio antes de que el intervalo quedara libre y tuvo que esperar.
     receivedAt del evento anterior se usa como aproximacion de su hora de publicacion.
  5. Calcula los intervalos entre items "sample" y los asocia al arranque (bootId =
     primera parte de eventId).
"""
import csv
import math
import statistics
from datetime import datetime, timedelta, timezone
from pathlib import Path

RAIZ = Path(__file__).resolve().parents[3]
PF04 = RAIZ / "docs/evidencias/pruebas/PF-04"
FUENTES = [PF04 / "historial-barrera.csv", PF04 / "historial-muestra.csv"]
SALIDA_CSV = RAIZ / "docs/evidencias/pruebas/PNF-05/historial-sesion.csv"
INTERVALO_MS = 10000
CRITERIO_D0_S = 15.0
BOLIVIA = timezone(timedelta(hours=-4))


def num(x):
    """Un decimal con coma, como en el informe."""
    return f"{x:.1f}".replace(".", ",")


def hora(ms):
    return datetime.fromtimestamp(ms / 1000, BOLIVIA).strftime("%Y-%m-%d %H:%M:%S")


def resumen(valores):
    v = sorted(valores)
    if not v:
        return "| 0 | — | — | — | — | — | — |"
    p95 = v[math.ceil(0.95 * len(v)) - 1]
    dentro = sum(x <= CRITERIO_D0_S for x in v)
    return (f"| {len(v)} | {num(v[0])} | {num(statistics.median(v))} | {num(statistics.mean(v))} | "
            f"{num(v[-1])} | {num(p95)} | {dentro}/{len(v)} |")


def main():
    filas, encabezado, repetidos = {}, None, 0
    for ruta in FUENTES:
        with open(ruta, encoding="utf-8-sig", newline="") as archivo:
            lector = csv.DictReader(archivo)
            encabezado = lector.fieldnames
            for fila in lector:
                if fila["eventKey"] in filas:
                    repetidos += 1
                filas[fila["eventKey"]] = fila
    total = len(filas)
    sinteticos = [f for f in filas.values() if f["eventId"].startswith("prueba")]
    eventos = sorted((f for f in filas.values() if not f["eventId"].startswith("prueba")),
                     key=lambda f: int(f["occurredAt"]))

    SALIDA_CSV.parent.mkdir(parents=True, exist_ok=True)
    with open(SALIDA_CSV, "w", encoding="utf-8", newline="") as archivo:
        escritor = csv.DictWriter(archivo, fieldnames=encabezado, quoting=csv.QUOTE_ALL)
        escritor.writeheader()
        escritor.writerows(eventos)

    print("# Análisis de las exportaciones de `SmartParkHistory`\n")
    print("Generado por [`analizar_historial.py`](analizar_historial.py) a partir de "
          "[`historial-barrera.csv`](../pruebas/PF-04/historial-barrera.csv) y "
          "[`historial-muestra.csv`](../pruebas/PF-04/historial-muestra.csv). "
          "Reproducción, desde la raíz del repositorio: "
          "`python docs/evidencias/analisis/analizar_historial.py > docs/evidencias/analisis/historial-resultados.md`; "
          "en Windows, con `PYTHONUTF8=1`.\n")
    print("## Depuración\n")
    print("| Paso | Ítems |\n|---|---:|")
    print(f"| Filas de ambos CSV | {total + repetidos} |")
    print(f"| `eventKey` repetidos entre los dos CSV | {repetidos} |")
    print(f"| Ítems únicos | {total} |")
    print(f"| Excluidos: `eventId` con prefijo `prueba` (publicados a mano desde el cliente MQTT) | {len(sinteticos)} |")
    print(f"| Eventos del firmware ([`historial-sesion.csv`](../pruebas/PNF-05/historial-sesion.csv)) | {len(eventos)} |\n")
    for f in sinteticos:
        d0 = (int(f["receivedAt"]) - int(f["occurredAt"])) / 1000
        print(f"- `{f['eventId']}`: `occurredAt` {hora(int(f['occurredAt']))}, Δ0 = {num(d0)} s (excluido).")
    print()

    conteo = {}
    for f in eventos:
        dia = hora(int(f["occurredAt"]))[:10]
        clave = (dia, f["eventType"], f["origin"])
        conteo[clave] = conteo.get(clave, 0) + 1
    print("## Eventos por día, tipo y origen (hora de Bolivia, UTC−4)\n")
    print("| Día | `eventType` | `origin` | Ítems |\n|---|---|---|---:|")
    for (dia, tipo, origen), n in sorted(conteo.items()):
        print(f"| {dia} | `{tipo}` | `{origen}` | {n} |")
    print()

    anterior_por_arranque, aislados, en_cola, detalle = {}, [], [], []
    for f in eventos:
        arranque = f["eventId"].split("-")[0]
        ocurrio, recibido = int(f["occurredAt"]), int(f["receivedAt"])
        d0 = (recibido - ocurrio) / 1000
        previo = anterior_por_arranque.get(arranque)
        cola = previo is not None and ocurrio < previo + INTERVALO_MS
        (en_cola if cola else aislados).append(d0)
        detalle.append((hora(ocurrio), f["eventType"], f["origin"], arranque[:8], d0, "en cola" if cola else "aislado"))
        anterior_por_arranque[arranque] = recibido
    print("## Δ0 = `receivedAt` − `occurredAt` (s)\n")
    print("Criterio complementario de RNF8: Δ0 ≤ 15 s. Un evento está **en cola** si ocurrió "
          "antes de que pasaran 10 s (`HISTORY_PUBLISH_INTERVAL_MS`) desde la publicación del "
          "evento anterior del mismo arranque; si no, está **aislado**.\n")
    print("| Grupo | n | Mín | Mediana | Promedio | Máx | P95 | ≤ 15 s |\n|---|---:|---:|---:|---:|---:|---:|---:|")
    print("| Aislados " + resumen(aislados))
    print("| En cola " + resumen(en_cola))
    print("| Todos " + resumen(aislados + en_cola))
    print()

    print("## Intervalos entre muestras (`sample`)\n")
    print("Criterio de RNF5: 300 ± 10 s (`HISTORY_SAMPLE_INTERVAL_MS`). Cada arranque publica una muestra al obtener la hora.\n")
    print("| Muestra | `occurredAt` | Arranque (`bootId`) | Intervalo (s) | Mismo arranque que la anterior |\n|---|---|---|---:|---|")
    muestras = [f for f in eventos if f["eventType"] == "sample"]
    for i, f in enumerate(muestras):
        arranque = f["eventId"].split("-")[0][:8]
        if i == 0:
            print(f"| {i + 1} | {hora(int(f['occurredAt']))} | `{arranque}` | — | — |")
            continue
        prev = muestras[i - 1]
        intervalo = (int(f["occurredAt"]) - int(prev["occurredAt"])) / 1000
        mismo = "Sí" if prev["eventId"].split("-")[0] == f["eventId"].split("-")[0] else "No (arranque nuevo)"
        print(f"| {i + 1} | {hora(int(f['occurredAt']))} | `{arranque}` | {num(intervalo)} | {mismo} |")
    arranques = sorted({f["eventId"].split("-")[0][:8] for f in muestras})
    print(f"\nArranques distintos entre las muestras: {len(arranques)}.\n")

    # El firmware numera cada evento generado (sequence_ se incrementa antes de
    # xQueueSend, src/HistoryPublisher.cpp#L41), incluso si la cola esta llena y el
    # evento se descarta. En un arranque cuyos eventos caen dentro del rango que cubre
    # la exportacion de lecturas (consulta por rango con los tres tipos), un numero de
    # secuencia ausente es un evento generado que no se almacenó.
    with open(FUENTES[1], encoding="utf-8-sig", newline="") as archivo:
        limite = max(int(f["occurredAt"]) for f in csv.DictReader(archivo)
                     if not f["eventId"].startswith("prueba"))
    print("## Integridad reconstruida por número de secuencia\n")
    print(f"`eventId` = `<bootId>-<secuencia>`. La exportación de lecturas cubre sin filtro de tipo "
          f"hasta {hora(limite)}; solo los arranques con todos sus eventos hasta esa hora permiten "
          "contar N (secuencias generadas) frente a M (ítems almacenados). En los demás arranques "
          "solo se exportaron los eventos de barrera.\n")
    print("| Arranque | Primer `occurredAt` | Secuencias | N | M | M/N | Cubierto por la exportación |\n|---|---|---|---:|---:|---:|---|")
    por_arranque = {}
    for f in eventos:
        arranque, secuencia = f["eventId"].split("-")
        por_arranque.setdefault(arranque, []).append((int(secuencia, 16), int(f["occurredAt"])))
    for arranque, valores in sorted(por_arranque.items(), key=lambda kv: min(o for _, o in kv[1])):
        secuencias = sorted(s for s, _ in valores)
        cubierto = max(o for _, o in valores) <= limite
        inicio = hora(min(o for _, o in valores))
        if not cubierto:
            print(f"| `{arranque[:8]}` | {inicio} | 0x{secuencias[0]:x}–0x{secuencias[-1]:x} | — | {len(secuencias)} | — | No (solo barrera) |")
            continue
        # La secuencia empieza en 1 en cada arranque (++sequence_ desde 0).
        ultimo = secuencias[-1]
        n, m = ultimo, len(secuencias)
        ausentes = [f"0x{s:x}" for s in range(1, ultimo + 1) if s not in secuencias]
        print(f"| `{arranque[:8]}` | {inicio} | 0x1–0x{ultimo:x} | {n} | {m} | {num(100 * m / n)} % | Sí |")
        if ausentes:
            print(f"|  | Ausentes: {', '.join(ausentes)} |  |  |  |  |  |")
    total_n = sum(max(s for s, _ in v) for v in por_arranque.values() if max(o for _, o in v) <= limite)
    total_m = sum(len(v) for v in por_arranque.values() if max(o for _, o in v) <= limite)
    print(f"| **Arranques cubiertos** |  |  | **{total_n}** | **{total_m}** | **{num(100 * total_m / total_n)} %** |  |")
    print()

    print("## Detalle por evento\n")
    print("| `occurredAt` | `eventType` | `origin` | Arranque | Δ0 (s) | Grupo |\n|---|---|---|---|---:|---|")
    for h, tipo, origen, arranque, d0, grupo in detalle:
        print(f"| {h} | `{tipo}` | `{origen}` | `{arranque}` | {num(d0)} | {grupo} |")


if __name__ == "__main__":
    main()
