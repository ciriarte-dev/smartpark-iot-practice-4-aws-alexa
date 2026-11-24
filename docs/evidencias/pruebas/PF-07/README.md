# PF-07 — Consultas y cambios de modo y antirrebote por voz

**Requisitos:** RF14, RF3. Procedimiento y criterio: [4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales).

Videos grabados por Isabella Montes en la sesión del 22/11/2026. La frase y la respuesta se leyeron en el chat del simulador; el cuadro indicado (índice / `pts_time`, [`tools/medir_videos.py`](../../../../tools/medir_videos.py)) es uno en que se leen. El chat acumula la conversación, así que un cuadro puede mostrar también las frases anteriores.

| Archivo | Intent | Frase | Respuesta observada (cuadro / s) | Coherencia |
|---|---|---|---|---|
| [`EspaciosDisponibles.mp4`](EspaciosDisponibles.mp4) | `ConsultarDisponiblesIntent` | *"cuántos espacios hay disponibles"* | *"El último reporte indica 4 espacios disponibles de 4."* (#75 / 2,500) | ✅ 4 LED verdes en #145 / 4,834 |
| [`EspaciosOcupados.mp4`](EspaciosOcupados.mp4) | `ConsultarOcupadosIntent` | *"cantidad de espacios ocupados"* | *"El último reporte indica 0 espacios ocupados de 4."* (#122 / 5,082) | ✅ Igual a la consulta anterior |
| [`Plaza2Ocupada-1.mp4`](Plaza2Ocupada-1.mp4), [`Plaza2Ocupada-2.mp4`](Plaza2Ocupada-2.mp4) | `ConsultarEstacionamientoIntent` | *"está libre la plaza dos"* | *"Según el último reporte, el estacionamiento 2 está ocupado."* (`-2`, #144 / 5,999) | ✅ LED rojo en la plaza 2 cubierta (`-1`, #10 / 0,333) |
| [`ConsultaModoParqueo.mp4`](ConsultaModoParqueo.mp4) | `ConsultarModoIntent` | *"en qué modo está el parqueo"* | *"El último modo reportado es automático."* (#290 / 9,700; también en `AbrirCuandoTodosEspaciosOcupados.mp4`, #389 / 13,000) | ✅ |
| [`ConsultaAntirrebote.mp4`](ConsultaAntirrebote.mp4) | `ConsultarAntirreboteIntent` | *"consulta el antirrebote"* | *"El antirrebote reportado es de 120 milisegundos."* (#231 / 7,700) | ✅ Valor por defecto |
| [`Antirrebote200ms.mp4`](Antirrebote200ms.mp4) | `CambiarAntirreboteIntent` | *"ajusta el antirrebote a doscientos milisegundos"* | *"Solicité 200 milisegundos de antirrebote. El dispositivo aún debe confirmarlo…"* (#191 / 6,397) | ✅ |
| [`Antirrebote120ms.mp4`](Antirrebote120ms.mp4) | `CambiarAntirreboteIntent` | *"ajusta el antirrebote a ciento veinte milisegundos"* | *"Solicité 120 milisegundos de antirrebote…"* (#286 / 9,567) | ✅ |
| [`Antirrebote120ms-2.mp4`](Antirrebote120ms-2.mp4) | — | *"ajusta el antirrebote a ciento veinte milisegundos"*, después del rechazo de 1001 ms | *"¿A qué hora quieres la alarma?"* (#163 / 5,467) | Repetición no válida: la sesión ya estaba cerrada (desviación 12, 5.5) |
| [`Antirrebote1001ms.mp4`](Antirrebote1001ms.mp4) | `CambiarAntirreboteIntent` | *"ajusta el antirrebote a mil uno milisegundos"* | *"Dime un valor entero entre 50 y 1000 milisegundos."* (#151 / 5,033) | ✅ Rechazado antes de escribir el Shadow ([`parking_handlers.py#L107-L109`](../../../../lambda/handlers/parking_handlers.py#L107-L109)) |
| [`Ayuda.mp4`](Ayuda.mp4) | `AMAZON.HelpIntent` | *"ayuda"* | *"Dentro de SmartPark puedes decir abrir, cerrar, consultar el estado de la barrera…"* (#281 / 9,373) | ✅ |
| [`Cancelar.mp4`](Cancelar.mp4) | `AMAZON.CancelIntent` | *"cancela"* | *"Hasta luego."* (#161 / 5,400) | ✅ |
| [`Detener.mp4`](Detener.mp4) | `AMAZON.StopIntent` | *"detener"* | *"Hasta luego."* (se lee en `AbrirCuandoTodosEspaciosOcupados.mp4`, #389 / 13,000) | ✅ |
| [`AbrirCuandoTodosEspaciosOcupados.mp4`](AbrirCuandoTodosEspaciosOcupados.mp4) | `AbrirBarreraIntent` con el parqueo lleno | *"abrir"* | Ver [PF-06](../PF-06/) | — |

**`Antirrebote120ms-2.mp4`.** El chat muestra *"ajusta el antirrebote a mil uno milisegundos"* → *"Dime un valor entero…"* y a continuación la frase de 120 ms sin volver a abrir la skill. **Hecho comprobado (código):** un valor fuera de rango lanza `UserError` y `SafeExceptionHandler` responde con `say()` sin `keep_open`, lo que cierra la sesión ([`system_handlers.py#L67`](../../../../lambda/handlers/system_handlers.py#L67)); la frase siguiente la interpreta el servicio general de Alexa. La misma frase dentro de una sesión (`Antirrebote120ms.mp4`) funciona.

**Resultado.** 11 frases de 9 intents con la respuesta esperada y 1 frase enviada fuera de la sesión; en las de plazas y modo, la respuesta coincide con los LED o la LCD. Las consultas de plazas, modo y antirrebote solo se responden con `reported` de menos de 180 s, por lo que también verifican RF2.

**Estado:** ✅ Cumple (consultas y ajuste del antirrebote). Observación: tras un valor rechazado hay que volver a invocar la skill.
