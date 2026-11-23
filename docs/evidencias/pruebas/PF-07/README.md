# PF-07 — Consultas y cambios de modo y antirrebote por voz

Esta carpeta contiene las evidencias en video de la prueba funcional **PF-07**, correspondiente a las consultas de plazas y al control por voz del parqueo.

Respalda los requerimientos **RF14 y RF3**. El procedimiento completo se describe en [PF-07 de la sección 4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales).

## Objetivo

Comprobar que Alexa responda correctamente a consultas sobre el estado de las plazas y que los resultados coincidan con el estado reportado por el dispositivo.

Los escenarios cubiertos son:

- Cantidad de espacios disponibles.
- Cantidad de espacios ocupados.
- Estado ocupado o libre de una plaza específica.
- Modo actual del parqueo.
- Antirrebote configurado.
- Cambio del antirrebote mediante voz.
- Validación de plazas y valores fuera de rango.
- Ayuda, cancelación y detención de la sesión de Alexa.

## Evidencias disponibles

| Evidencia | Consulta demostrada | Resultado |
|---|---|---|
| [EspaciosDisponibles.mp4](<EspaciosDisponibles.mp4>) | *"¿Cuántos espacios hay disponibles?"* | Evidencia de la consulta de plazas disponibles |
| [EspaciosOcupados.mp4](<EspaciosOcupados.mp4>) | *"¿Cuántas plazas están ocupadas?"* | Evidencia de la consulta de plazas ocupadas |
| [Plaza2Ocupada-1.mp4](<Plaza2Ocupada-1.mp4>) | Consulta del estado de la plaza 2 | Evidencia de la respuesta sobre una plaza ocupada |
| [Plaza2Ocupada-2.mp4](<Plaza2Ocupada-2.mp4>) | Consulta del estado de la plaza 2 | Evidencia adicional de la respuesta sobre una plaza ocupada |
| [ConsultaModoParqueo.mp4](<ConsultaModoParqueo.mp4>) | Consulta del modo actual del parqueo | Evidencia de la respuesta con el modo reportado |
| [ConsultaAntirrebote.mp4](<ConsultaAntirrebote.mp4>) | Consulta del valor actual del antirrebote | Evidencia de la respuesta con el parámetro configurado |
| [Antirrebote200ms.mp4](<Antirrebote200ms.mp4>) | Ajuste válido del antirrebote a 200 ms | Evidencia de la solicitud por voz |
| [Antirrebote120ms.mp4](<Antirrebote120ms.mp4>) | Ajuste válido del antirrebote a 120 ms | Evidencia de la solicitud por voz |
| [Antirrebote120mss.mp4](<Antirrebote120mss.mp4>) | Ajuste válido del antirrebote a 120 ms | Evidencia adicional del ajuste por voz |
| [Antirrebote1001ms.mp4](<Antirrebote1001ms.mp4>) | Ajuste fuera de rango a 1001 ms | Evidencia del rechazo de la validación |
| [Ayuda.mp4](<Ayuda.mp4>) | Solicitud de ayuda | Evidencia del manejo de `AMAZON.HelpIntent` |
| [Cancelar.mp4](<Cancelar.mp4>) | Cancelación de la sesión | Evidencia del manejo de `AMAZON.CancelIntent` |
| [Detener.mp4](<Detener.mp4>) | Detención de la sesión | Evidencia del manejo de `AMAZON.StopIntent` |
| [AbrirCuandoTodosEspaciosOcupados.mp4](<AbrirCuandoTodosEspaciosOcupados.mp4>) | Intento de abrir la barrera con todas las plazas ocupadas | Evidencia de la relación entre el estado completo y el control de la barrera |

## Resultados observados

| Escenario | Evidencia | Estado |
|---|---|---|
| Consulta de espacios disponibles | `EspaciosDisponibles.mp4` | Evidencia registrada |
| Consulta de espacios ocupados | `EspaciosOcupados.mp4` | Evidencia registrada |
| Consulta de la plaza 2 | `Plaza2Ocupada-1.mp4` y `Plaza2Ocupada-2.mp4` | Evidencia registrada |
| Consulta del modo del parqueo | `ConsultaModoParqueo.mp4` | Evidencia registrada |
| Consulta del antirrebote | `ConsultaAntirrebote.mp4` | Evidencia registrada |
| Ajuste del antirrebote a 200 ms | `Antirrebote200ms.mp4` | Evidencia registrada |
| Ajuste del antirrebote a 120 ms | `Antirrebote120ms.mp4` y `Antirrebote120mss.mp4` | Evidencias registradas |
| Rechazo de 1001 ms | `Antirrebote1001ms.mp4` | Evidencia registrada |
| Solicitud de ayuda | `Ayuda.mp4` | Evidencia registrada |
| Cancelación | `Cancelar.mp4` | Evidencia registrada |
| Detención | `Detener.mp4` | Evidencia registrada |
| Apertura con el parqueo lleno | `AbrirCuandoTodosEspaciosOcupados.mp4` | Evidencia registrada |

## Evidencias complementarias recomendadas

- Capturas del simulador para documentar el intent y el slot de cada frase.
- Capturas del Shadow para confirmar los cambios válidos del antirrebote.
- Captura de la respuesta ante la plaza 5 o el valor 1001 ms.
- Capturas separadas de ayuda, cancelación y detención si se requiere mayor trazabilidad.

## Criterios de aprobación

PF-07 se considera aprobada cuando se verifica:

1. Las consultas reportan el estado correspondiente al `reported` del dispositivo.
2. El modo y el antirrebote pueden consultarse por voz.
3. Los valores válidos modifican el parámetro mediante `desired` y son confirmados en `reported`.
4. Los valores fuera de rango no escriben el Shadow.
5. Las consultas inválidas reciben una respuesta apropiada.
6. Ayuda mantiene la sesión y cancelar o detener finalizan la interacción.
