# PF-06 — Lógica Edge y convivencia con la voz

Respalda RF13 (E1–E3), RF14. Procedimiento, resultado esperado y criterio: [PF-06 en 4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales). Los valores se anotan primero en la [hoja de registro](../../hojas-registro/hoja-pruebas-funcionales.md).

| Archivo esperado | Cómo obtenerlo |
|---|---|
| `../serial-sesion.txt` (log común de la sesión) | Log del monitor serie de toda la sesión: `pio device monitor -f time \| tee docs/evidencias/pruebas/serial-sesion.txt` en Git Bash, iniciado en el bloque 0 (4.1). |
| `update-accepted-*.png` | Captura del cliente de prueba MQTT de AWS IoT suscrito a `$aws/things/smartpark-grupo9-01/shadow/update/accepted` en cada paso del procedimiento (bloqueo, cierre, apertura con plaza libre, modo manual). |
| `simulador-*.png` | Capturas del simulador de Alexa (**Test → Development**) con la respuesta y el panel **JSON Input** de cada frase. |

## Texto esperado en la LCD 16×2

| Paso de PF-06 | Fila 0 | Fila 1 |
|---|---|---|
| `automatico` con las plazas 1–4 ocupadas; *"abre la barrera"*, `OPEN` y *"cierra la barrera"* | `Libres:0/4 Oc:4` | `APERTURA BLOQ.` |
| Plaza 4 liberada y apertura aceptada | `Libres:1/4 Oc:3` | Alterna cada 2 s: `Modo: automatico` / `Barrera: abierta` |
| Plaza 4 ocupada de nuevo, `manual` y apertura aceptada | `Libres:0/4 Oc:4` | `PARQUEO LLENO` |

Las rutas con `../` apuntan a archivos compartidos con otras pruebas. Esta carpeta se completa durante la sesión de banco; no contiene resultados todavía.
