# SmartPark V2 · Alexa y Device Shadow

La Fase 4 amplía la skill existente de parqueo inteligente. Para RF13, el ESP32 conserva la decisión de permitir o bloquear la apertura según el modo y las plazas confirmadas. La Lambda de Alexa transmite los ajustes solicitados a AWS IoT Core. Para RF14, las consultas leen `state.reported` y los cambios actualizan `state.desired`.

## Intents de la Fase 4

| Intent | Frase de prueba | Atributo |
|---|---|---|
| `ConsultarDisponiblesIntent` | Cuántos espacios hay disponibles | `reported.disponibles` y `reported.capacidad` |
| `ConsultarOcupadosIntent` | Cuántos espacios están ocupados | `reported.ocupados` y `reported.capacidad` |
| `ConsultarEstacionamientoIntent` | Cómo está el estacionamiento dos | `reported.slots.2` |
| `ConsultarModoIntent` | En qué modo está el parqueo | `reported.modo` |
| `ActivarModoAutomaticoIntent` | Activa el modo automático | `desired.modo = "automatico"` |
| `ActivarModoManualIntent` | Activa el modo manual | `desired.modo = "manual"` |
| `ConsultarAntirreboteIntent` | Consulta el antirrebote | `reported.parametros_auto.antirrebote_ir_ms` |
| `CambiarAntirreboteIntent` | Ajusta el antirrebote a ciento veinte milisegundos | `desired.parametros_auto.antirrebote_ir_ms = 120` |

El firmware acepta enteros de 50 a 1.000 ms. Las plazas se consultan con números del uno al cuatro. Si una plaza reporta `desconocido`, Alexa indica que falta una lectura confirmada. La consulta comprueba el timestamp de cada atributo utilizado, incluidos los atributos anidados. El límite existente es de 180 segundos. Un reporte reciente describe la última observación y puede seguir disponible después de una desconexión breve. El éxito de una escritura significa que AWS recibió la solicitud. Si el ESP32 está desconectado, el ajuste puede quedar pendiente hasta que se sincronice. Consultar de nuevo permite verificar el valor reportado.

Cada solicitud usa la vinculación del usuario de Alexa en `SmartParkBindings`. Los permisos existentes en el rol de `SmartParkAlexaBackend` cubren `iot:GetThingShadow` e `iot:UpdateThingShadow` sobre los Things autorizados. El rol sigue usando los permisos existentes de vinculación y registro. Si se configura `IOT_DATA_ENDPOINT`, el backend usa ese hostname; la búsqueda automática del endpoint requiere el permiso existente `iot:DescribeEndpoint`. [Política IAM de ejemplo](../lambda/iam/alexa-execution-policy.example.json)

## Despliegue manual

1. En AWS Lambda, actualiza el código de **SmartParkAlexaBackend** con `lambda/dist/SmartParkAlexaBackend.zip`. El ZIP se genera localmente con `BUILD_OFFLINE=1 sh lambda/scripts/build.sh alexa` si las dependencias ya están en `lambda/build/wheels`. Se carga el ZIP completo en lugar de archivos sueltos.
2. Mantén Python 3.13, el handler `lambda_function.lambda_handler`, el trigger Alexa Skills Kit asociado a la skill y los valores actuales de `ALEXA_SKILL_ID`, `TABLE_NAME=SmartParkBindings` e `IOT_DATA_ENDPOINT` si ya se usa.
3. En Alexa Developer Console, abre la skill existente, confirma el idioma y guarda una copia del contenido de **Build → JSON Editor**. El informe de Práctica 3 documenta **Spanish (US)**, código `es-US`. La configuración actual de la consola se debe cotejar antes de importar.
4. El modelo desplegado en la consola es [`alexa/es-US.smartpark-v2.final.json`](../alexa/es-US.smartpark-v2.final.json): combina los intents de la Práctica 3 con los ocho de la Fase 4 y se importa completo en **Build → JSON Editor**. Los nuevos slots `espacio` y `milisegundos` usan `AMAZON.NUMBER`. [Tipos de slots de Alexa](https://developer.amazon.com/en-US/docs/alexa/custom-skills/slot-type-reference.html) La prueba [`test_alexa_model.py`](../lambda/tests/test_alexa_model.py) valida su nombre de invocación, que cada intent propio tenga handler y que las frases sean únicas y usen solo slots declarados. El repositorio conserva [`alexa/es-US.interaction-model.json`](../alexa/es-US.interaction-model.json) como modelo inicial de la Fase 4; no es el modelo que se carga en la consola.
5. Pulsa **Save Model** y **Build Model**. En **Test**, habilita **Development** y abre la skill con el nombre de invocación del modelo activo. [Editor y compilación del modelo](https://developer.amazon.com/en-US/docs/alexa/custom-skills/custom-interaction-model-reference.html)

## Validación desde la consola

- Confirma en **JSON Input** el intent esperado para cada frase de la tabla. Para las plazas, prueba del uno al cuatro y revisa el valor de `espacio`. Para antirrebote, revisa `milisegundos` con 49, 50, 120, 1.000 y 1.001.
- Con una cuenta vinculada, compara disponibles, ocupados y estado por plaza con la LCD o con el último `reported`. Repite con una cuenta sin vinculación y comprueba que Alexa solicite vincular. Con dos cuentas vinculadas a Things distintos, verifica que cada cambio se dirija al Thing de su cuenta.
- Solicita el modo automático. Comprueba que Alexa diga que lo solicitó, verifica `desired.modo` y después consulta hasta que `reported.modo` refleje el cambio. Repite para el modo manual y para el antirrebote. Los valores inválidos deben quedar fuera del Shadow.
- Con el ESP32 desconectado, confirma que la orden no se anuncie como aplicada. Tras el periodo de frescura, la consulta debe indicar que el reporte está desactualizado. Prueba también el estado de barrera, la distancia, el vehículo, la vinculación y la desvinculación.
- Revisa en el simulador posibles confusiones entre “estado de la barrera” y “estado del estacionamiento”, “consulta el modo” y “activa el modo”, y “consulta el antirrebote” y “ajusta el antirrebote”. La revisión local comprobó que las muestras fueran únicas y que sus slots estuvieran declarados. El reconocimiento de voz queda pendiente de la consola.

## Verificación local

Pasaron 105 pruebas Python sin conexiones AWS, incluidas las de Práctica 3, las comprobaciones de consultas, cambios, aislamiento y vigencia de datos, la orden de barrera en formato V1 (`desired.barrera`) y el modelo final ([`test_alexa_model.py`](../lambda/tests/test_alexa_model.py)). Salida: [`lambda-unittest.txt`](evidencias/pruebas-automaticas/lambda-unittest.txt). El ZIP extraído pasó 80 pruebas adicionales y su handler raíz se pudo importar.

## Auditoría de voz y vinculación

El archivo para importar es [es-US.smartpark-v2.final.json](../alexa/es-US.smartpark-v2.final.json). Conserva los 16 intents propios, los cuatro de Amazon y los nombres y tipos de los slots. Se añadió «consultar estado» y se sustituyeron frases genéricas de desvinculación por «elimina la vinculación de mi parqueo inteligente» y «quita la vinculación de mi parqueo inteligente». También sigue disponible «desvincula mi parqueo». El reconocimiento de estas frases con micrófono requiere validación en Alexa.

| Observación | Hallazgo local | Comprobación en AWS o Alexa |
|---|---|---|
| Distancia sin dato válido | El firmware retira `reported.distancia_cm` cuando el HC-SR04 no obtiene una medición válida. El backend distingue ahora el campo ausente del reporte desactualizado y sigue exigiendo el timestamp del propio campo. | Ejecutar `DISTANCE` con un objeto dentro de rango, consultar el Shadow clásico del Thing vinculado y comparar el valor y timestamp de `distancia_cm`. |
| Plaza 2 responde «Esta función todavía no está disponible» | El handler existe y está registrado en la copia local. Un número fuera de 1–4 es inválido. Si falta la plaza y `capacidad` reciente es menor al número pedido, el handler indica que esa plaza no está configurada. | Capturar `request.intent.name`, `slots.espacio.value` y la salida del simulador. Si el intent es correcto, cotejar el ARN, alias y versión de la Lambda. Revisar `reported.slots.2` y su timestamp. |
| Desvincular activa Bluetooth | Las expresiones genéricas pueden salir de la skill. El modelo ya ofrece frases explícitas que nombran el parqueo inteligente. | Abrir la skill, comprobar el saludo y decir la frase explícita. Si Alexa responde sobre Bluetooth y no aparece la solicitud de la skill en Skill I/O, revisar invocación e idioma antes de revisar Lambda. |
| Órdenes de barrera e histórico | El backend envía el objetivo V1 en `desired.barrera`. Esa orden carece de ID y origen de voz explícito. El firmware conserva compatibilidad con el formato V2. | Comprueba `desired.barrera`, `reported.barrera` y el movimiento. El histórico registra las órdenes V1 como `legado`; Serial se registra como `edge`. Una apertura bloqueada no debe generar movimiento. |

**PIN y DynamoDB.** `SmartParkPairingAdmin` comprueba que el Thing exista y emite un PIN de 4 dígitos entre 1000 y 9999, con vigencia de 300 s y hasta 3 intentos. Guarda la autorización y su ventana en una transacción de DynamoDB. `SmartParkAlexaBackend` valida el formato y el usuario de Alexa, descuenta el intento, verifica vigencia y elegibilidad y resuelve el Thing. Una transacción consume el PIN y guarda o cambia la vinculación. El cambio comprueba la revisión anterior para evitar sobrescribir una vinculación concurrente. Desvincular elimina solo la asociación del usuario. Un código de 12 dígitos es incompatible con el validador y con `AMAZON.FOUR_DIGIT_NUMBER`. Si el sistema desplegado muestra un código de 12 dígitos, identifica de qué pantalla o función salió antes de considerar una migración. Conserva el código administrativo vigente y comparte únicamente la longitud o una captura enmascarada. [Slots de Alexa](https://developer.amazon.com/en-US/docs/alexa/custom-skills/slot-type-reference.html)

### Matriz de compatibilidad y pruebas

En **Recognition / Skill I/O**, registra el texto reconocido, `locale`, intent y slots antes de evaluar la respuesta. Todos los handlers nombrados en la tabla están registrados en el `SkillBuilder` local. En pruebas físicas de barrera, usa modo manual y luego repite la apertura en automático con las cuatro plazas ocupadas para verificar bloqueo. La posición del servo en el Shadow es una estimación temporal.

| Intent esperado | Frase de prueba y slot | Handler | Acción y respuesta Lambda | Shadow esperado | Comprobación ESP32 | Prueba local |
|---|---|---|---|---|---|---|
| AbrirBarreraIntent | abre la barrera | AbrirBarreraHandler | Solicitud aceptada por IoT | desired.barrera=abierta (V1) | Apertura en manual. Bloqueo en automático lleno. | test_open_and_close_only_desired |
| CerrarBarreraIntent | cierra la barrera | CerrarBarreraHandler | Solicitud aceptada por IoT | desired.barrera=cerrada (V1) | Cierre explícito y evento si cambia el estado estimado | test_open_and_close_only_desired |
| ConsultarDistanciaIntent | consulta la distancia | ConsultarDistanciaHandler | Lee IoT y responde en cm | reported.distancia_cm reciente | Comparar con DISTANCE | test_valid_distance_reported_not_desired |
| ConsultarVehiculoIntent | hay un auto esperando | ConsultarVehiculoHandler | Lee IoT y responde presencia, ausencia o dato desconocido | reported.vehiculo_detectado reciente | Colocar y retirar un objeto | test_presence_true_and_false |
| EstadoBarreraIntent | consulta el estado de la barrera | EstadoBarreraHandler | Lee IoT y responde la última posición reportada | reported.barrera reciente | Comparar con la posición observada | test_barrier_state_reported |
| VincularParqueoIntent | vincula mi parqueo con el código, slot codigo de 4 dígitos | VincularParqueoHandler | DynamoDB e IoT DescribeThing. Consume PIN y guarda asociación. | — | — | test_valid_binding_and_one_use_code |
| CambiarVinculacionIntent | cambia mi vinculación con el código, slot codigo de 4 dígitos | CambiarVinculacionHandler | DynamoDB e IoT DescribeThing. Consume PIN y cambia asociación. | — | — | test_change_binding |
| DesvincularParqueoIntent | elimina la vinculación de mi parqueo inteligente | DesvincularParqueoHandler | DynamoDB. Elimina solo la asociación del usuario. | — | — | test_unbind_one_delete_no_get |
| ConsultarDisponiblesIntent | cuántos espacios hay disponibles | AvailableSpacesHandler | Lee IoT y responde el conteo | reported.disponibles y capacidad | Comparar con LCD | test_v2_reads_reported_counts_mode_and_debounce |
| ConsultarOcupadosIntent | cuántas plazas están ocupadas | OccupiedSpacesHandler | Lee IoT y responde el conteo | reported.ocupados y capacidad | Comparar con LCD | test_v2_reads_reported_counts_mode_and_debounce |
| ConsultarEstacionamientoIntent | está libre la plaza dos, slot espacio=2 | ParkingSlotHandler | Lee IoT. Estado, lectura desconocida o plaza no configurada. | reported.slots.2 y capacidad si falta la plaza | Comparar con LED, repetir para 1–4 | test_v2_reads_each_slot_and_rejects_invalid_numbers |
| ConsultarModoIntent | en qué modo está el parqueo | ParkingModeHandler | Lee IoT y anuncia modo reportado | reported.modo | Comparar con LCD | test_v2_reads_reported_counts_mode_and_debounce |
| ActivarModoAutomaticoIntent | activa el modo automático | ActivateAutomaticHandler | Solicitud aceptada por IoT | desired.modo=automatico, luego reported | Confirmar en LCD y verificar bloqueo con 4 ocupadas | test_v2_writes_precise_patches_and_uses_each_users_binding |
| ActivarModoManualIntent | activa el modo manual | ActivateManualHandler | Solicitud aceptada por IoT | desired.modo=manual, luego reported | Confirmar en LCD | test_v2_writes_precise_patches_and_uses_each_users_binding |
| ConsultarAntirreboteIntent | consulta el antirrebote | ReadDebounceHandler | Lee IoT y responde en ms | reported.parametros_auto.antirrebote_ir_ms | — | test_v2_reads_reported_counts_mode_and_debounce |
| CambiarAntirreboteIntent | ajusta el antirrebote a doscientos milisegundos, slot milisegundos=200 | SetDebounceHandler | Solicitud aceptada por IoT para 50–1000 | desired.parametros_auto.antirrebote_ir_ms, luego reported | Confirmar aplicación en Shadow | test_v2_writes_precise_patches_and_uses_each_users_binding y test_v2_rejects_out_of_range_debounce_and_unbound_user |
| AMAZON.HelpIntent | ayuda | HelpHandler | Ayuda y sesión abierta | — | — | test_help_and_fallback_are_clear_and_do_not_call_aws |
| AMAZON.CancelIntent | cancela | ExitHandler | Despedida y cierre de sesión | — | — | test_stop_and_cancel_close_without_aws_calls |
| AMAZON.StopIntent | detente | ExitHandler | Despedida y cierre de sesión | — | — | test_stop_and_cancel_close_without_aws_calls |
| AMAZON.NavigateHomeIntent | ve a inicio, en un dispositivo con pantalla | Alexa lo gestiona | Sale de la skill en el dispositivo compatible | — | — | Presencia en modelo local. Navegación requiere dispositivo con pantalla. |

### Orden de validación y respaldo

1. Exporta el modelo actual de Alexa para `es-US`. En Lambda identifica el ARN usado por el endpoint de la skill, incluido alias si lo hay. Guarda el ZIP actual y el hash de código de esa versión. Guarda localmente un inventario del handler, runtime, rol, trigger y nombres de variables. Evita capturar valores sensibles. Mantén `ALEXA_SKILL_ID`, `TABLE_NAME=SmartParkBindings` e `IOT_DATA_ENDPOINT` según la configuración existente.
2. Abre **Test → Development → Alexa Simulator**, selecciona **Spanish (US)** y habilita **Skill I/O**. Por texto, escribe «abre parqueo inteligente» y prueba primero distancia, plaza y modo. Si obtienes el intent correcto en JSON Input, compara la salida Lambda, el ARN configurado y el Shadow del Thing vinculado.
3. Repite con micrófono. Amazon documenta entrada de voz en el simulador. El permiso del micrófono de esta sesión y el idioma seleccionado deben verificarse en el navegador. Autoriza el micrófono a `developer.amazon.com`, selecciona el dispositivo de entrada y pulsa el micrófono para hablar. En el simulador puedes probar «abre parqueo inteligente» y, tras el saludo, el comando. Si el micrófono no funciona, usa un Echo con una cuenta autorizada para probar la skill, configura el idioma Spanish (US) y di «Alexa, abre parqueo inteligente». Prueba NavigateHome en un Echo con pantalla. [Simulador](https://developer.amazon.com/en-US/docs/alexa/devconsole/alexa-simulator.html) · [Pruebas en un Echo](https://developer.amazon.com/en-US/docs/alexa/test/test-your-skill-overview.html) · [Intents integrados](https://developer.amazon.com/en-US/docs/alexa/custom-skills/standard-built-in-intents.html)
4. Cuando autorices el despliegue manual, carga `lambda/dist/SmartParkAlexaBackend.zip` en la función usada por la skill y prueba primero las consultas. Importa el JSON final, guarda y compila en la etapa de desarrollo. Valida por texto y por voz los cambios de modo, las órdenes de barrera y, por último, la vinculación con una cuenta de pruebas. Comprueba bloqueos, órdenes vencidas y el origen del histórico con la maqueta.
