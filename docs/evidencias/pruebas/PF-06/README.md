# PF-06 — Lógica Edge y convivencia con la voz

Esta carpeta contiene las evidencias de la prueba funcional **PF-06**, correspondiente a la lógica de control ejecutada en el Edge y a su convivencia con las órdenes por voz.

Respalda **RF13 (E1–E3)** y **RF14**. El procedimiento completo se describe en [PF-06 de la sección 4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales).

## Objetivo

Comprobar que el ESP32:

- Siga el estado de las cuatro plazas mediante LEDs y LCD.
- Bloquee las aperturas en modo `automatico` cuando no haya plazas disponibles.
- Permita siempre el cierre de la barrera.
- Permita nuevamente la apertura al liberarse una plaza.
- Conviva con los cambios de modo realizados por Alexa.

Las reglas verificadas son:

- **E1:** indicación local de cada plaza y conteo de libres y ocupadas.
- **E2:** bloqueo de apertura en modo `automatico` con cero plazas libres.
- **E3:** cierre aceptado independientemente de la ocupación.

## Evidencias disponibles

| Evidencia | Función demostrada | Resultado |
|---|---|---|
| [Abrir.mp4](<Abrir.mp4>) | Apertura de la barrera | Evidencia registrada |
| [Abrir1.mp4](<Abrir1.mp4>) | Apertura de la barrera | Evidencia registrada |
| [Abrir2.mp4](<Abrir2.mp4>) | Apertura de la barrera | Evidencia registrada |

Los videos presentan escenarios de apertura registrados durante la ejecución de PF-06. Deben relacionarse con el estado de ocupación y el modo mostrado en la maqueta.

## Evidencia por escenario

| Video | Uso recomendado |
|---|---|
| [Abrir.mp4](<Abrir.mp4>) | Mostrar una apertura en un escenario concreto de PF-06 |
| [Abrir1.mp4](<Abrir1.mp4>) | Mostrar una segunda ejecución o verificación de apertura |
| [Abrir2.mp4](<Abrir2.mp4>) | Mostrar una tercera ejecución o verificación de apertura |

Al revisar los videos debe cotejarse el movimiento de la barrera con la LCD y con el estado de ocupación correspondiente.

## Escenarios de PF-06

| Escenario | Resultado esperado |
|---|---|
| Cuatro plazas ocupadas en modo `automatico` | Barrera inmóvil al solicitar apertura; orden bloqueada; LCD `APERTURA BLOQ.` |
| Cierre con las cuatro plazas ocupadas | El cierre se acepta y la barrera se cierra |
| Liberación de la plaza 4 | La LCD muestra `Libres:1/4 Oc:3` y la apertura vuelve a permitirse |
| Parqueo lleno y modo `manual` | Apertura aceptada con LCD `PARQUEO LLENO` |
| Apertura por voz o por `OPEN` Serial con cero libres | Bloqueada mientras no exista una plaza libre confirmada |

## Texto esperado en la LCD 16×2

| Paso de PF-06 | Fila 0 | Fila 1 |
|---|---|---|
| `automatico` con las plazas 1–4 ocupadas; apertura y cierre solicitados | `Libres:0/4 Oc:4` | `APERTURA BLOQ.` |
| Plaza 4 liberada y apertura aceptada | `Libres:1/4 Oc:3` | Alterna cada 2 s: `Modo: automatico` / `Barrera: abierta` |
| Plaza 4 ocupada de nuevo, `manual` y apertura aceptada | `Libres:0/4 Oc:4` | `PARQUEO LLENO` |

## Criterios de aprobación

PF-06 se considera aprobada cuando se verifica:

1. Los LEDs y la fila 0 de la LCD siguen el estado de las plazas.
2. Con cero plazas libres en `automatico`, la apertura se rechaza y se muestra `APERTURA BLOQ.`.
3. El cierre siempre se acepta.
4. Al liberarse una plaza, la apertura vuelve a aceptarse.
5. En modo `manual`, la barrera puede abrirse aunque todas las plazas estén ocupadas.
