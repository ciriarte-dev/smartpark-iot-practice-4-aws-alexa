# SmartPark V2 · Historial AWS

Preparación local de la Fase 3. Las plantillas de [`aws/history`](../aws/history/) requieren sustituir `<REGION>` y `<ACCOUNT_ID>` antes del despliegue manual. El informe de Práctica 3 se conserva en [`docs/informe.md`](informe.md).

## Trazabilidad del enunciado

| Requisito | Estado |
|---|---|
| RF10 | IoT Rule local para `smartpark/+/history/v1` y acción Lambda. Publicador del ESP32 integrado y probado con dobles. Pendiente la prueba en AWS. |
| RF11 | Lambda y pruebas locales de tipos, rangos, coherencia de conteos, origen del Thing y marcas de tiempo. Pendiente comprobar la escritura en AWS. |
| RF12 | Contrato y pruebas de transiciones de barrera con origen y posición estimada. El firmware genera las transiciones al finalizar el tiempo de recorrido. Pendiente validar con el servo físico y AWS. |
| RF13 | El bloqueo por disponibilidad del firmware corre en Edge para mantener latencia baja y operación sin conexión. La Lambda almacena datos y carece de permisos para accionar la barrera. La validación física sigue pendiente. |
| RF14 | El firmware ya reporta modo y antirrebote. Consulta y cambio por voz corresponden a la Fase 4. |
| RF15 | El historial admite consultas por Thing y tiempo, filtrables por tipo de evento. Los mockups de utilización y actividad se completarán en la Fase 5. |

## Contrato del tópico

Ruta `smartpark/<thingName>/history/v1`. La regla extrae el nombre del Thing con `topic(2)` y la versión con `topic(4)`. Añade `receivedAt`, la hora de recepción en milisegundos UTC. Los campos de identidad del mensaje MQTT se toman del tópico autorizado. La política de certificado restringe cada dispositivo a su ruta y requiere asociación con el Thing. Si alguna política existente permite publicar en rutas más amplias, revisar ese permiso antes de activar el histórico. [Funciones de AWS IoT SQL](https://docs.aws.amazon.com/iot/latest/developerguide/iot-sql-functions.html) · [Variables de política del Thing](https://docs.aws.amazon.com/iot/latest/developerguide/thing-policy-variables.html)

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

- `sample` guarda una instantánea. `occupancy_changed` guarda la misma instantánea más `data.slot` (número de plaza) y `data.estado_anterior`, que debe diferir del nuevo estado en `slots`. Se admiten capacidades de una a cuatro plazas. `desconocido` queda fuera de los conteos de libre y ocupado. La distancia válida está entre 2 y 400 cm. En ausencia de lectura, enviar `null` para distancia y presencia. Con distancia válida se puede enviar presencia `null` mientras se estabiliza.
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
  "origin": "voz",
  "data": {
    "anterior": "abierta",
    "actual": "cerrada",
    "posicion_estimada": true,
    "requestId": "voice-close"
  }
}
```

Para una transición de plaza se usa el formato de muestra de la sección Contrato del tópico, con `eventType: "occupancy_changed"` y los campos `data.slot` y `data.estado_anterior`. `requestId` se omite para movimientos Serial o de Shadow legado sin ID de pedido. En las pruebas, el histórico llegó a **347 B** de JSON y **394 B** con la reserva de cabecera MQTT. El límite comprobado en compilación admite un JSON de hasta 511 B más tópico y cabecera. El paquete de respuesta `GET` del Shadow sigue midiendo **2.518 B** en la prueba representativa. El búfer compartido se mantiene en **3.072 B**.

## Despliegue manual posterior

Usar la misma región para el Thing, la regla, la Lambda y la tabla. Las acciones de esta sección están pendientes de autorización.

1. Crear `SmartParkHistory` con [`table.example.json`](../aws/history/table.example.json). Confirmar el estado `ACTIVE`.
2. Crear el grupo CloudWatch Logs `/aws/lambda/SmartParkHistoryProcessor` y fijar retención, por ejemplo 14 días. Crear un rol de ejecución con [la confianza de Lambda existente](../lambda/iam/execution-role-trust.example.json) y [la política del procesador](../aws/history/processor-execution-policy.example.json). La política permite escribir en esa tabla y en los streams del grupo precreado.
3. Crear la Lambda `SmartParkHistoryProcessor`, Python 3.13, handler `history_lambda_function.lambda_handler`, 128 MB como punto de partida y variable `HISTORY_TABLE_NAME=SmartParkHistory`. Empaquetar solo el archivo de este procesador en la raíz del ZIP desde `lambda/` con `zip -j SmartParkHistoryProcessor.zip history_lambda_function.py`. Utiliza el SDK Boto3 del runtime. Las versiones de memoria y tiempos son supuestos hasta que se mida su ejecución.
4. Añadir el permiso de invocación a la Lambda con [`iot-invoke-permission.example.json`](../aws/history/iot-invoke-permission.example.json), limitado al ARN de `SmartParkHistoryIngest` y a la cuenta. Crear `SmartParkHistoryIngest` deshabilitada con [`iot-rule.example.json`](../aws/history/iot-rule.example.json). [Permiso de AWS IoT para invocar Lambda](https://docs.aws.amazon.com/lambda/latest/dg/services-iot.html)
5. Adjuntar [la política complementaria de publicación](../aws/history/device-publish-policy.example.json) al certificado asociado al Thing. Mantener la política Shadow del dispositivo. Confirmar que `clientId` coincide con el nombre del Thing.
6. Habilitar la regla y publicar casos de humo en un Thing de pruebas autorizado. Verificar una lectura válida, un duplicado idéntico con una sola fila, una transición de barrera y el descarte de un mensaje inválido. Consultar por rango y revisar `Errors` y `Throttles` de Lambda. Para una prueba prolongada, evaluar un destino de fallos asíncronos antes de depender del historial.
7. Con el despliegue y la validación eléctrica autorizados, cargar el firmware y probar desconexión, reinicio y recuperación. Comparar las transiciones estimadas del ESP32 con las filas del histórico y con el movimiento observado. Deshabilitar la regla para detener el flujo si falla la validación o el consumo supera el previsto. La tabla conserva las filas ya escritas.

## Costo incremental ilustrativo

Escenario de 30 días con un Thing y una muestra cada cinco minutos, más 20 eventos diarios. Son 9.240 mensajes/mes de hasta 5 KB, una acción por regla y un ítem de hasta 1 KB por evento. Se supone 100 ms de Lambda x86 a 128 MB. Se usan tarifas unitarias públicas de referencia, antes de créditos, para estimar el orden de magnitud. Confirmar la región y tarifas en AWS Pricing Calculator al autorizar el despliegue.

| Concepto | Supuesto unitario | USD/mes aprox. |
|---|---:|---:|
| MQTT entrante | 1,00 por millón | 0,0092 |
| IoT Rule más una acción | 0,30 por millón | 0,0028 |
| Invocaciones Lambda | 0,20 por millón | 0,0018 |
| Cómputo Lambda | 0,0000166667 por GB-s | 0,0019 |
| Escrituras DynamoDB Standard bajo demanda | 0,625 por millón | 0,0058 |
| **Total del flujo** | | **0,022** |

El estimado excluye conectividad, Shadow y Alexa existentes, lecturas del histórico, almacenamiento acumulado, logs, reintentos e impuestos. Los duplicados y mensajes inválidos pueden seguir causando invocaciones y los reintentos pueden aumentar el consumo. Las tarifas citadas ilustran el cálculo y no constituyen un precio contratado. [Precios de AWS IoT Core](https://aws.amazon.com/iot-core/pricing/) · [Precios de AWS Lambda](https://aws.amazon.com/lambda/pricing/) · [Precios de DynamoDB bajo demanda](https://aws.amazon.com/dynamodb/pricing/on-demand/)

La capacidad del prototipo se define en un solo valor, `PARKING_CAPACITY` de `include/config.h`. La plantilla mantiene cuatro plazas activas. Un valor menor usa las primeras plazas del mismo mapa y se refleja en LEDs, LCD, conteos y Shadow.

## Pruebas locales

Desde `lambda/`, ejecutar `AWS_EC2_METADATA_DISABLED=true python -m unittest discover -s tests` con las dependencias existentes. El procesador se prueba con Botocore Stubber: escritura esperada, duplicado, propagación de error transitorio y descarte sin inicializar el SDK. Las plantillas también se verifican sin acceso a AWS.

Desde la raíz, ejecutar también `SMARTPARK_HISTORY_PYTHON=/ruta/al/python-con-boto3 sh test/run_host_tests.sh`. El ejecutable C++ exporta los mensajes que realmente entrega al doble MQTT y el validador Python añade los metadatos de la regla simulada. Este ensayo valida los tipos de eventos, los orígenes conocidos y que el reintento reutilice la misma clave.
