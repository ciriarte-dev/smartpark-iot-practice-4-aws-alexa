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

| Evidencia | Función probada |
|---|---|
| [Abrir-Cerrar.mp4](<Abrir-Cerrar.mp4>) | Órdenes de voz para abrir y cerrar la barrera |
| [ConsultarDistancia.mp4](<ConsultarDistancia.mp4>) | Consulta de la distancia |
| [AutoEsperando.mp4](<AutoEsperando.mp4>) | Consulta de vehículo esperando |
| [ConsultarEstado.mp4](<ConsultarEstado.mp4>) | Consulta del estado de la barrera |
| [pf02.xlsx](<pf02.xlsx>) | Pruebas de detección vehicular por distancia |

## Resultado observado

| Escenario | Evidencia | Resultado |
|---|---|---|
| Apertura y cierre de la barrera mediante comandos de voz | `Abrir-Cerrar.mp4` | La barrera responde a las órdenes enviadas por Alexa |
| Consulta de distancia | `ConsultarDistancia.mp4` | Alexa entrega la respuesta de la consulta de distancia |
| Consulta de vehículo esperando | `AutoEsperando.mp4` | Alexa entrega el resultado de la detección de un vehículo |
| Consulta del estado de la barrera | `ConsultarEstado.mp4` | Alexa entrega el estado actual de la barrera |
| Lectura por umbral | `pf02.xlsx` | Se registraron cuatro distancias en varias pruebas |

## Resultado

**Estado: Ejecutada.** Los cuatro escenarios de PF-02 cuentan con una evidencia en video adjunta.

## Criterios de aprobación

PF-02 se considera demostrada cuando las evidencias muestran:

1. Órdenes de apertura y cierre recibidas por Alexa y ejecutadas por la barrera.
2. Una respuesta a la consulta de distancia.
3. Una respuesta a la consulta de vehículo esperando.
4. Una respuesta con el estado actual de la barrera.