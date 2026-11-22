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
