# Evidencias integradas PF-03 a PF-06

Esta carpeta reúne una evidencia consolidada utilizada en las pruebas funcionales **PF-03, PF-04, PF-05 y PF-06**.

Las pruebas deben consultarse y verificarse por separado según su alcance:

- **PF-03:** vinculación y cambio de vinculación del usuario con el parqueo.
- **PF-04:** publicación y almacenamiento del histórico en `SmartParkHistory`.
- **PF-05:** validación de eventos inválidos y duplicados.
- **PF-06:** lógica Edge de ocupación, bloqueo de apertura y convivencia con la voz.

La organización individual de las pruebas está documentada en sus carpetas cuando existen:

- [`PF-04`](../PF-04/)
- [`PF-06`](../PF-06/)

## Evidencia disponible

| Evidencia | Contenido | Pruebas relacionadas |
|---|---|---|
| [PF04 - pf05.xlsx](<PF04 - pf05.xlsx>) | Tres ejecuciones de apertura y cierre mediante comandos por voz, con el estado `Desired` observado | PF-04 y PF-05 |

## Contenido de la hoja consolidada

La hoja contiene tres pruebas consecutivas:

| Prueba | Comando por voz | Estado `Desired` esperado |
|---|---|---|
| Prueba 1 | Alexa, abre parqueo inteligente / cerrar la barrera | Abierta / Cerrado |
| Prueba 2 | Alexa, abre parqueo inteligente / cerrar la barrera | Abierta / Cerrado |
| Prueba 3 | Alexa, abre parqueo inteligente / cerrar la barrera | Abierta / Cerrado |

La hoja permite contrastar el comando recibido por Alexa con el estado solicitado al Device Shadow.

## Alcance por prueba

### PF-03 — Vinculación

Debe comprobarse el PIN temporal, la vinculación, el cambio de vinculación y la desvinculación del usuario. Esta carpeta no contiene actualmente una evidencia específica de PF-03.

### PF-04 — Histórico

Debe comprobarse que cada transición publicada genere un evento válido en `SmartParkHistory`. Las exportaciones existentes se encuentran en [`PF-04`](../PF-04/).

Los eventos deben incluir `thingName`, `eventKey`, `schemaVersion = v1`, `occurredAt`, `receivedAt` y `data`. Los cambios de barrera deben conservar el origen `voz`, `edge`, `nube` o `legado` según corresponda.

### PF-05 — Validación y duplicados

Debe comprobarse que los mensajes inválidos no sean almacenados y que un duplicado no cree un segundo ítem. Esta carpeta no contiene actualmente capturas, mensajes ni CSV específicos de PF-05.

### PF-06 — Lógica Edge

Debe comprobarse el bloqueo de apertura en modo `automatico`, el cierre siempre permitido y la indicación correcta en LEDs y LCD. Las evidencias disponibles se encuentran en [`PF-06`](../PF-06/).

## Criterios generales

La evidencia consolidada se considera útil cuando permite demostrar:

1. La correspondencia entre la orden por voz y el estado `Desired`.
2. La apertura y el cierre posteriores del actuador.
3. La existencia de registros coherentes en el histórico.
4. La separación entre eventos válidos, inválidos y duplicados.
5. La decisión tomada por el Edge ante el estado de las plazas.
