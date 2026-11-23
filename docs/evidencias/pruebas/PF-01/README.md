# PF-01 — Lectura de sensores y reporte al Shadow

Esta carpeta contiene las evidencias de la prueba funcional **PF-01**, destinada a verificar la lectura de distancia, el estado de la plaza 1 y el reporte al Device Shadow.

Respalda los requerimientos **RF1 y RF2**. El procedimiento completo se describe en [PF-01 de la sección 4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales).

## Objetivo

Comprobar que el ESP32:

- Ejecute las consultas Serial `DISTANCE` y `SLOT1`.
- Mida distancias de 10, 20 y 30 cm.
- Detecte la presencia de un vehículo en el rango configurado.
- Cambie la plaza 1 de libre a ocupada al cubrir el sensor IR.
- Libere la plaza al retirar el objeto.
- Publique el estado actualizado en el Shadow.

## Evidencias registradas

| Evidencia | Contenido |
|---|---|
| [Video de la sesión](<WhatsApp Video 2026-11-22 at 19.32.05 (1).mp4>) | Secuencia registrada de PF-01 |
| [pf01.xlsx](<pf01.xlsx>) | Seis pruebas de distancia y detección vehicular |
| [pf01-10cm.jpeg](<pf01-10cm.jpeg>) | Medición a 10 cm |
| [pf01-10cmResultado.jpeg](<pf01-10cmResultado.jpeg>) | Resultado de la medición a 10 cm |
| [pf01-20cm.jpeg](<pf01-20cm.jpeg>) | Medición a 20 cm |
| [pf01-20cmResultado.jpeg](<pf01-20cmResultado.jpeg>) | Resultado de la medición a 20 cm |
| [pf01-30cm.jpeg](<pf01-30cm.jpeg>) | Medición a 30 cm |
| [pf01-30cmResultado.jpeg](<pf01-30cmResultado.jpeg>) | Resultado de la medición a 30 cm |

## Datos registrados

| Paso | Hora | Resultado observado | ¿Cumple? |
|---|---|---|---|
| Enviar `DISTANCE` y `SLOT1` | — | Los comandos se ejecutaron | Sí |
| Cubrir y liberar el IR de la plaza 1; revisar `update/accepted` | 19:24:41 (UTC-04:00) | La plaza cambió el estado de libre a ocupado | Sí |

## Lecturas de distancia

La hoja `pf01.xlsx` registra seis pruebas con objetos a 10, 20 y 30 cm. Las mediciones registradas están próximas a las distancias reales y deben contrastarse con las tolerancias definidas en PNF-02.

La captura de 10 cm corresponde a una lectura con vehículo detectado. Las mediciones a 20 y 30 cm sirven como respaldo adicional de la respuesta del sensor ultrasónico.

## Evidencias pendientes

| Archivo | Evidencia |
|---|---|
| `../serial-sesion.txt` | Log común del monitor serie iniciado antes de la sesión. |
| `update-accepted.png` | Captura MQTT suscrita a `$aws/things/smartpark-grupo9-01/shadow/update/accepted`. |

## Criterios de aprobación

1. `DISTANCE` muestra una distancia válida y coherente con el objeto.
2. `SLOT1` muestra el estado de la plaza 1.
3. Al cubrir y liberar el IR, el estado del Shadow pasa de libre a ocupado y vuelve a libre.
4. Los resultados quedan respaldados por el video, las capturas y la hoja de mediciones.
