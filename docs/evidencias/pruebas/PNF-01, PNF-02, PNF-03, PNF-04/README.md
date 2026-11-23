# Evidencias integradas PNF-01 a PNF-04

Esta carpeta reúne el registro principal de las pruebas no funcionales **PNF-01, PNF-02, PNF-03 y PNF-04**.

Las pruebas verifican estabilidad, exactitud de sensores, tiempo de respuesta por voz y latencia del control automático en el Edge.

Los procedimientos y criterios completos se describen en [las pruebas no funcionales de la sección 4.3 del informe](../../../informe-practica-4.md#43-pruebas-no-funcionales).

## Evidencias disponibles

| Evidencia | Pruebas relacionadas | Contenido |
|---|---|---|
| [registro-no-funcionales.pdf](<registro-no-funcionales.pdf>) | PNF-01, PNF-02, PNF-03 y PNF-04 | Registro impreso o anotado de mediciones no funcionales |

## PNF-01 — Estabilidad

**Requerimiento:** RNF1.

Debe medirse durante toda la sesión:

- Reinicios del ESP32.
- Reconexiones no provocadas.
- Bloqueos o falta de respuesta del sistema.
- Duración aproximada de la ejecución.

Evidencia requerida:

- `STATUS` inicial y final.
- Hora de inicio y fin.
- Conteo de reinicios, reconexiones y bloqueos.

Criterio: cero reinicios, cero reconexiones no provocadas y cero bloqueos.

## PNF-02 — Exactitud de sensores

**Requerimientos:** RNF2 y RNF3.

Debe comprobarse:

- Exactitud del sensor HC-SR04 a 10, 20 y 30 cm.
- Coincidencia entre el vehículo real, el sensor IR, el LED y el estado registrado en las cuatro plazas.

### HC-SR04

Se requieren cinco lecturas por distancia.

| Distancia real | Criterio |
|---|---|
| 10 cm | Error absoluto máximo dentro de 1 cm |
| 20 cm | Error absoluto máximo dentro de 1 cm |
| 30 cm | Error absoluto máximo dentro de 1 cm |

### Sensor IR

Se requieren cinco entradas y cinco salidas por plaza.

| Indicador | Criterio |
|---|---|
| Aciertos IR | Al menos 38 de 40 |
| Porcentaje | Al menos 95 % |

## PNF-03 — Respuesta por voz

**Requerimiento:** RNF4.

Debe medirse el tiempo desde que aparece o se reconoce la orden por voz hasta que la barrera se detiene.

La medición se realiza con el video de PF-02, contando cuadros a una frecuencia conocida.

| Indicador | Criterio |
|---|---|
| Tres aperturas | Tiempo medido por cada repetición |
| Tres cierres | Tiempo medido por cada repetición |
| Tiempo máximo | 3,0 segundos |

La hoja de registro contiene seis repeticiones: tres órdenes `abre` y tres órdenes `cierra`.

## PNF-04 — Lógica automática en el Edge

**Requerimiento:** RNF9.

Debe medirse el tiempo desde que el vehículo cubre el sensor IR hasta el cambio del LED.

También debe verificarse en el video:

- Actualización de la fila 0 de la LCD en cada ocupación.
- Cuarta ocupación con `Libres:0/4 Oc:4`.
- Fila 1 con `APERTURA BLOQ.`.

| Indicador | Criterio |
|---|---|
| Cinco repeticiones | Cuatro plazas y una nueva ocupación |
| Tiempo máximo | 0,3 segundos |

## Evidencias complementarias pendientes

| Evidencia | Uso |
|---|---|
| Video PNF-02 o capturas del banco | Medición de distancia y exactitud del IR |
| Video PNF-03 | Conteo de cuadros para latencia voz-servo |
| Video PNF-04 | Conteo de cuadros IR-LED y verificación de LCD |
| Log serie | Tiempos y estados de PNF-01 y PNF-02 |
| Hoja de cálculo o exportación | Consolidación de las mediciones |

## Criterios generales

Las cuatro pruebas se consideran aprobadas cuando:

1. PNF-01 registra estabilidad sin reinicios ni reconexiones no provocadas.
2. PNF-02 cumple la tolerancia del sensor ultrasónico y el porcentaje de aciertos del IR.
3. PNF-03 mantiene la respuesta por voz dentro de 3 segundos.
4. PNF-04 mantiene la respuesta local IR-LED dentro de 0,3 segundos.
