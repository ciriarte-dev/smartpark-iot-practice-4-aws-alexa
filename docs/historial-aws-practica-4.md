# SmartPark V2 · Historial AWS

Preparación local de la Fase 3. Las plantillas de [`aws/history`](../aws/history/) requieren sustituir `<REGION>` y `<ACCOUNT_ID>` antes del despliegue manual. Esta práctica extiende la Práctica 3: [repositorio de la Práctica 3](https://github.com/HenryRiveraM/smartpark-practica-3).

## Trazabilidad del enunciado

| Requisito | Estado |
|---|---|
| RF10 | ✅ Cumple: en AWS, la regla `SmartParkHistoryIngest` invocó `SmartParkHistoryProcessor` y quedaron almacenados en `SmartParkHistory` los 31 eventos de barrera de la sesión del 22/11 ([informe, 4.2](informe-practica-4.md#42-pruebas-funcionales), [5.1](informe-practica-4.md#51-matriz-de-cumplimiento) y [5.3](informe-practica-4.md#53-datos-almacenados-en-dynamodb)). |
| RF11 | ⚠️ Cumple parcial: las 46 lecturas almacenadas en AWS tienen `thingName`, `schemaVersion` y `receivedAt`, pero son del 19 y 20/11 con `capacidad = 1`, y el descarte de inválidos y duplicados solo está verificado con PA-03 ([informe, 5.3](informe-practica-4.md#53-datos-almacenados-en-dynamodb) y fichas [F12](informe-practica-4.md#f12-lecturas-almacenadas-pf-04-rf11) y [F6](informe-practica-4.md#f6-integridad-del-almacenamiento-pf-05-y-pnf-05)). |
| RF12 | ✅ Cumple: con el servo físico y AWS se almacenaron 38 `barrier_changed` con su origen, 31 `legado` (órdenes V1, incluida la voz) y 7 `edge` (`OPEN`/`CLOSE` por Serial) ([informe, 4.2](informe-practica-4.md#42-pruebas-funcionales) y [5.3](informe-practica-4.md#53-datos-almacenados-en-dynamodb)). |
| RF13 | ⚠️ Cumple parcial: en el hardware, E1 cumple en video (LED y LCD según las plazas), E2 tiene evidencia parcial (bloqueo con 0 libres y desbloqueo con 1 libre, sin filmar el rechazo de una orden), y E3 y E4 solo están verificadas con PA-02 ([informe, 4.2](informe-practica-4.md#42-pruebas-funcionales), [5.1](informe-practica-4.md#51-matriz-de-cumplimiento) y fichas [F10](informe-practica-4.md#f10-reglas-del-edge-pf-06) y [F8](informe-practica-4.md#f8-operación-sin-conexión-y-e4-pnf-06)). |
| RF14 | El firmware reporta modo y antirrebote; la consulta y el cambio por voz están en [`parking_handlers.py`](../lambda/handlers/parking_handlers.py) ([alexa-practica-4.md](alexa-practica-4.md)). |
| RF15 | El historial admite consultas por Thing y tiempo, filtrables por tipo de evento. Mockups M1–M4 en [`docs/mockups/`](mockups/) y sus consultas en el [informe, 2.12](informe-practica-4.md#212-diseño-de-reportes-mockups). |

## Contrato del tópico

Ruta `smartpark/<thingName>/history/v1`. La regla extrae el nombre del Thing con `topic(2)` y la versión con `topic(4)`. Añade `receivedAt`, la hora de recepción en milisegundos UTC. Los campos de identidad del mensaje MQTT se toman del tópico autorizado. La plantilla de la política del certificado ([`device-publish-policy.example.json`](../aws/history/device-publish-policy.example.json)) restringe cada dispositivo a su ruta y requiere asociación con el Thing. En la cuenta, el certificado tiene `SmartParkPolicy` con `iot:*`, que no aplica esa restricción; el riesgo y la migración están en el [informe, 3.2](informe-practica-4.md#32-configuraciones-en-aws-y-permisos-asignados). [Funciones de AWS IoT SQL](https://docs.aws.amazon.com/iot/latest/developerguide/iot-sql-functions.html) · [Variables de política del Thing](https://docs.aws.amazon.com/iot/latest/developerguide/thing-policy-variables.html)

Ejemplo de lectura MQTT. Generar `occurredAt` con un reloj UTC sincronizado. `eventId` admite de 1 a 64 caracteres ASCII alfanuméricos, guion o guion bajo. Reutilizar el mismo ID y la misma hora de ocurrencia en todo reenvío.

```json
{
  "eventId": "boot76-001",
  "occurredAt": 1795000000000,
  "eventType": "sample",
  "origin": "edge",
  "data": {
    "slots": {"1":"libre","2":"ocupado","3":"desconocido","4":"libre"},
    "capacidad": 4,
    "ocupados": 1,
    "disponibles": 2,
    "modo": "manual",
    "distancia_cm": 12.4,
    "vehiculo_detectado": true
  }
}
```

- `sample` guarda una instantánea. `occupancy_changed` guarda la misma instantánea más `data.slot` (número de plaza) y `data.estado_anterior`, que debe diferir del nuevo estado en `slots`. Se admiten capacidades de una a cuatro plazas. `desconocido` queda fuera de los conteos de libre y ocupado. La distancia válida está entre 2 y 400 cm. En ausencia de lectura, enviar `null` para distancia y presencia. El validador admite presencia `null` con distancia válida; el firmware actual envía la presencia de la misma muestra que la distancia.
- `barrier_changed` guarda `data.anterior`, `data.actual`, `data.posicion_estimada: true` y, cuando exista, `data.requestId`. El estado inicial puede ser `desconocida`. El evento identifica una transición de estado estimado. Las órdenes rechazadas y las que repiten el estado actual no cuentan como movimientos. El origen es `voz`, `edge`, `nube` o `legado`. Este último conserva la falta de procedencia explícita del comando Shadow V1.
- La Lambda descarta eventos con tipos, rangos o combinaciones incorrectas y horas de ocurrencia más de cinco minutos por delante de la hora recibida. El tiempo de recepción no reemplaza al de ocurrencia. Los errores de infraestructura se propagan para el reintento asíncrono. Un duplicado se considera procesado.

## Datos y consultas

`SmartParkHistory` usa capacidad bajo demanda, `thingName` (String) como clave de partición y `eventKey` (String) como clave de ordenamiento. La clave es `<occurredAt en 13 dígitos>#<eventId>`. Una escritura condicional mantiene la primera fila si llega el mismo evento varias veces, incluso si cambia `receivedAt`. La garantía de deduplicación depende de que el productor preserve juntos ID y hora de ocurrencia. La tabla está separada de `SmartParkBindings`. [Escrituras condicionales de DynamoDB](https://docs.aws.amazon.com/amazondynamodb/latest/developerguide/Expressions.ConditionExpressions.html)

Para un intervalo inclusivo, usar `Query` con igualdad de `thingName` y `eventKey BETWEEN <desde13># AND <hasta13>#~`. Recorrer todas las páginas mediante `LastEvaluatedKey`. Se pueden filtrar las transiciones por `eventType` y los movimientos por `origin`. El filtro aplica después de leer el intervalo y no reduce su costo. Calcular utilización por duración entre transiciones, con muestras como apoyo para identificar lagunas. Tratar los períodos desconocidos como cobertura faltante y separar los intervalos en los que cambie la capacidad. La plantilla inicial no incorpora GSI ni TTL. Evaluar retención e índices después de medir volumen y patrón de consultas.

## Publicador integrado en el firmware

El bucle local registra transiciones a partir de los estados filtrados de las plazas activas y toma una muestra inicial tras disponer de hora válida. Emite como máximo una muestra periódica cada cinco minutos. Las peticiones de barrera que inician un pulso asocian su origen e ID en ese momento. Una transición se registra después del tiempo de recorrido si el estado estimado cambia. El primer movimiento parte de `desconocida`. Los objetivos repetidos y las órdenes rechazadas no generan eventos. Si una orden nueva interrumpe un movimiento, solo se registra la transición cuyo recorrido completó el temporizador y cambió el último estado estimado. El SG90 de la maqueta sigue sin realimentación de posición.

El envío al tópico usa la misma tarea MQTT del Shadow, después de atenderlo. Su intervalo mínimo es de 10 s entre intentos, con presupuesto de publicación separado. Bajo conexión continua esto limita el histórico a 8.640 intentos diarios, con hasta 288 muestras periódicas diarias. Se mantienen los límites y la reserva de control del Shadow.

| Situación | Comportamiento |
|---|---|
| Cola llena | Hay tres colas FIFO en RAM: 8 transiciones de barrera, 16 de plazas y 1 muestra. La tarea envía primero barrera, luego plazas y luego muestras. Cada cola conserva lo que ya tenía y descarta el evento nuevo cuando está llena. El bucle usa operaciones de cola sin espera. |
| Pérdida de Wi-Fi | Se sigue capturando mientras hay hora válida y capacidad. La tarea reanuda el envío al recuperar MQTT y sincronizar el Shadow. Se aplica el límite de 10 s durante el drenado. Si termina la ventana de demostración heredada, la tarea espera el siguiente reinicio. |
| Error de publicación local | Se reintenta hasta tres intentos totales, preservando los mismos bytes, ID y hora. Agotado el límite, se descarta el evento y avanza la cola. |
| Reinicio | Se pierden los eventos pendientes en RAM. Después de recuperar la hora, el firmware crea un identificador aleatorio de arranque y genera IDs nuevos con una secuencia. Las transiciones anteriores a la primera sincronización de hora se contabilizan y se descartan. |
| Resultado MQTT `true` | El cliente aceptó la escritura local y se retira el evento de la cola. PubSubClient publica con QoS 0. No existe acuse de aplicación ni confirmación de la escritura en DynamoDB. Un corte puede perder un mensaje que el cliente ya retiró. |

`STATUS` muestra envíos MQTT aceptados localmente, cola llena, eventos sin hora, fallos de envío y descartes de red. La deduplicación de DynamoDB cubre entregas repetidas con el mismo `eventId` y `occurredAt`. Para exigir confirmación de almacenamiento haría falta un acuse emitido después del `PutItem` o del duplicado, más persistencia de la cola para sobrevivir reinicios. Eso queda fuera de este publicador. [PubSubClient](https://github.com/knolleary/pubsubclient/blob/master/README.md)

El formato MQTT tiene exactamente `eventId`, `occurredAt`, `eventType`, `origin` y `data`. Los IDs emitidos concatenan 32 dígitos hexadecimales de arranque, un guion y 16 dígitos de secuencia. Ejemplo de barrera:

```json
{
  "eventId": "b1982cbcd114f5cc81e4945fce01eac1-0000000000000011",
  "occurredAt": 1795000000000,
  "eventType": "barrier_changed",
  "origin": "legado",
  "data": {
    "anterior": "abierta",
    "actual": "cerrada",
    "posicion_estimada": true
  }
}
```

Para una transición de plaza se usa el formato de muestra de la sección Contrato del tópico, con `eventType: "occupancy_changed"` y los campos `data.slot` y `data.estado_anterior`. El ejemplo corresponde a una orden de voz de la skill, que llega en formato V1 y se registra como `legado`. `requestId` solo se incluye con una orden V2 (`orden_barrera.id`); se omite para movimientos Serial o de Shadow legado. En las pruebas, el histórico llegó a **347 B** de JSON y **394 B** con la reserva de cabecera MQTT. El límite comprobado en compilación admite un JSON de hasta 511 B más tópico y cabecera. El paquete de respuesta `GET` del Shadow mide **2.519 B** en la prueba representativa. El búfer compartido se mantiene en **3.072 B**.

## Despliegue manual

Usar la misma región para el Thing, la regla, la Lambda y la tabla. Estos pasos son el procedimiento con el que se desplegaron los recursos del histórico en `us-east-1`; la configuración y los permisos asignados en la cuenta están en el [informe, 3.2](informe-practica-4.md#32-configuraciones-en-aws-y-permisos-asignados).

1. Crear `SmartParkHistory` con [`table.example.json`](../aws/history/table.example.json). Confirmar el estado `ACTIVE`.
2. Crear el grupo CloudWatch Logs `/aws/lambda/SmartParkHistoryProcessor` y fijar retención, por ejemplo 14 días. Crear un rol de ejecución con [la confianza de Lambda existente](../lambda/iam/execution-role-trust.example.json) y [la política del procesador](../aws/history/processor-execution-policy.example.json). La política permite escribir en esa tabla y en los streams del grupo precreado.
3. Crear la Lambda `SmartParkHistoryProcessor`, Python 3.13, handler `history_lambda_function.lambda_handler`, 128 MB como punto de partida y variable `HISTORY_TABLE_NAME=SmartParkHistory`. Empaquetar solo el archivo de este procesador en la raíz del ZIP desde `lambda/` con `zip -j SmartParkHistoryProcessor.zip history_lambda_function.py`. Utiliza el SDK Boto3 del runtime. Las versiones de memoria y tiempos son supuestos hasta que se mida su ejecución.
4. Añadir el permiso de invocación a la Lambda con [`iot-invoke-permission.example.json`](../aws/history/iot-invoke-permission.example.json), limitado al ARN de `SmartParkHistoryIngest` y a la cuenta. Crear `SmartParkHistoryIngest` deshabilitada con [`iot-rule.example.json`](../aws/history/iot-rule.example.json). [Permiso de AWS IoT para invocar Lambda](https://docs.aws.amazon.com/lambda/latest/dg/services-iot.html)
5. Adjuntar [la política complementaria de publicación](../aws/history/device-publish-policy.example.json) al certificado asociado al Thing. Mantener la política Shadow del dispositivo. Confirmar que `clientId` coincide con el nombre del Thing. En la cuenta, el certificado usa `SmartParkPolicy` (`iot:*`) en lugar de estas dos políticas.
6. Habilitar la regla y publicar casos de humo en un Thing de pruebas autorizado. Verificar una lectura válida, un duplicado idéntico con una sola fila, una transición de barrera y el descarte de un mensaje inválido. Consultar por rango y revisar `Errors` y `Throttles` de Lambda. Para una prueba prolongada, evaluar un destino de fallos asíncronos antes de depender del historial.
7. Cargar el firmware y probar desconexión, reinicio y recuperación. Comparar las transiciones estimadas del ESP32 con las filas del histórico y con el movimiento observado. Deshabilitar la regla para detener el flujo si falla la validación o el consumo supera el previsto. La tabla conserva las filas ya escritas.

## Costo

El costo mensual de todo el sistema (Shadow, histórico, Lambdas y DynamoDB), con tarifas verificadas de `us-east-1`, está en el [informe, 5.6](informe-practica-4.md#56-costo-mensual-estimado).

La capacidad del prototipo se define en un solo valor, `PARKING_CAPACITY` de `include/config.h`. La plantilla mantiene cuatro plazas activas. Un valor menor usa las primeras plazas del mismo mapa y se refleja en LEDs, LCD, conteos y Shadow.

## Pruebas locales

Desde `lambda/`, ejecutar `AWS_EC2_METADATA_DISABLED=true python -m unittest discover -s tests` con las dependencias existentes. El procesador se prueba con Botocore Stubber: escritura esperada, duplicado, propagación de error transitorio y descarte sin inicializar el SDK. Las plantillas también se verifican sin acceso a AWS.

Desde la raíz, ejecutar también `SMARTPARK_HISTORY_PYTHON=/ruta/al/python-con-boto3 sh test/run_host_tests.sh`. El ejecutable C++ exporta los mensajes que realmente entrega al doble MQTT y el validador Python añade los metadatos de la regla simulada. Este ensayo valida los tipos de eventos, los orígenes conocidos y que el reintento reutilice la misma clave.
