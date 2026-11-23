# PF-02 — Barrera y consultas por voz

Esta carpeta contiene las evidencias en video de la prueba funcional **PF-02**, correspondiente al control de la barrera y a las consultas de estado por voz mediante Alexa.

Respalda los requerimientos **RF3, RF4, RF5 y RF6**. El procedimiento completo se describe en [PF-02 de la sección 4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales).

## Objetivo

Comprobar que Alexa pueda:

- Abrir y cerrar la barrera.
- Consultar la distancia medida por el sensor ultrasónico.
- Consultar si hay un vehículo esperando.
- Consultar el estado actual de la barrera.

## Evidencias registradas

| Evidencia | Función probada | Resultado |
|---|---|---|
| [Abrir-Cerrar.mp4](<Abrir-Cerrar.mp4>) | Órdenes de voz para abrir y cerrar la barrera | Evidencia de control de la barrera por Alexa |
| [ConsultarDistancia.mp4](<ConsultarDistancia.mp4>) | *"Consulta la distancia"* | Evidencia de la respuesta de Alexa a la consulta de distancia |
| [AutoEsperando.mp4](<AutoEsperando.mp4>) | *"Hay un auto esperando"* | Evidencia de la consulta sobre la detección de un vehículo |
| [ConsultarEstado.mp4](<ConsultarEstado.mp4>) | *"Consulta el estado de la barrera"* | Evidencia de la respuesta sobre el estado de la barrera |

## Resultado observado

| Escenario | Evidencia | Resultado |
|---|---|---|
| Apertura y cierre de la barrera mediante comandos de voz | `Abrir-Cerrar.mp4` | La barrera responde a las órdenes enviadas por Alexa |
| Consulta de distancia | `ConsultarDistancia.mp4` | Alexa entrega la respuesta de la consulta de distancia |
| Consulta de vehículo esperando | `AutoEsperando.mp4` | Alexa entrega el resultado de la detección de un vehículo |
| Consulta del estado de la barrera | `ConsultarEstado.mp4` | Alexa entrega el estado actual de la barrera |

## Resultado

**Estado: Ejecutada.** Los cuatro escenarios de PF-02 cuentan con una evidencia en video adjunta.

## Criterios de aprobación

PF-02 se considera demostrada cuando las evidencias muestran:

1. Órdenes de apertura y cierre recibidas por Alexa y ejecutadas por la barrera.
2. Una respuesta a la consulta de distancia.
3. Una respuesta a la consulta de vehículo esperando.
4. Una respuesta con el estado actual de la barrera.