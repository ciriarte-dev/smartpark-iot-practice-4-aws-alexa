# PF-04 — Almacenamiento del histórico

Esta carpeta contiene las evidencias de la prueba funcional **PF-04**, correspondiente al registro y almacenamiento de eventos en `SmartParkHistory`.

Respalda los requerimientos **RF10, RF11 y RF12**. El procedimiento completo se describe en [PF-04 de la sección 4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales).

## Objetivo

Comprobar que los eventos del sistema sean recibidos, validados y almacenados en DynamoDB con los atributos requeridos:

- `thingName`
- `eventKey`
- `schemaVersion = v1`
- `occurredAt`
- `receivedAt`
- `data`

También se verifica que cada cambio de estado de la barrera conserve su origen:

- `voz`, para órdenes enviadas desde Alexa.
- `edge`, para órdenes `OPEN` y `CLOSE` enviadas desde el terminal Serial.
- `nube` o `legado`, según corresponda a otras fuentes admitidas.

## Evidencia registrada

| Evidencia | Procedimiento demostrado | Estado |
|---|---|---|
| [Abrir-CerrraPorTerminal.mp4](<Abrir-CerrraPorTerminal.mp4>) | Apertura y cierre de la barrera mediante las órdenes `OPEN` y `CLOSE` enviadas por el terminal Serial | Evidencia registrada |
| [historial-barrera.csv](<historial-barrera.csv>) | Exportación de eventos `barrier_changed` | Evidencia registrada |
| [historial-muestra.csv](<historial-muestra.csv>) | Exportación de eventos `sample` y `occupancy_changed` | Evidencia registrada |

El video documenta que los comandos `OPEN` y `CLOSE` se enviaron por el terminal y que la barrera realizó los cambios correspondientes.

## Datos del histórico

Los CSV contienen registros exportados de `SmartParkHistory`:

- `historial-barrera.csv`: cambios de estado de la barrera, estados anterior/actual y origen del evento.
- `historial-muestra.csv`: muestras del parqueo y cambios de ocupación.

Los registros incluyen `thingName`, `eventKey`, `eventType`, `schemaVersion`, `occurredAt`, `receivedAt` y `data`. Los eventos de barrera registrados incluyen orígenes `edge` y `legado`; las muestras y ocupaciones tienen origen `edge`.

## Criterios de aprobación

PF-04 se considera aprobada cuando se verifica:

1. Que exista una invocación del procesador por cada evento publicado.
2. Que los ítems contengan los atributos obligatorios.
3. Que los datos almacenados sean coherentes con los eventos observados.
4. Que los cambios de barrera registren el origen correcto.
5. Que los resultados de DynamoDB y CloudWatch permitan reconstruir la sesión.
