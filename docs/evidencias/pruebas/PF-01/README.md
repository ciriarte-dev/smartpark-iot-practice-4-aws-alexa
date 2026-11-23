# PF-01 — Lectura de sensores y reporte al Shadow

Respalda RF1, RF2. Procedimiento, resultado esperado y criterio: [PF-01 en 4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales). Los valores se anotan primero en la [hoja de registro](../../hojas-registro/hoja-pruebas-funcionales.md).

| Archivo esperado | Cómo obtenerlo |
|---|---|
| `../serial-sesion.txt` (log común de la sesión) | Log del monitor serie de toda la sesión: `pio device monitor -f time \| tee docs/evidencias/pruebas/serial-sesion.txt` en Git Bash, iniciado en el bloque 0 (4.1). |
| `update-accepted.png` | Captura del cliente de prueba MQTT de AWS IoT suscrito a `$aws/things/smartpark-grupo9-01/shadow/update/accepted` tras cubrir y liberar el IR de la plaza 1. |

## Comprobación del ultrasónico y del Classic Shadow

1. Abre el monitor a 115200 baudios y registra la marca `AWS Shadow diagnostico v1 compilado=`, el Thing y el `STATUS` inicial. En el cliente MQTT suscríbete a `update/accepted` y `update/rejected` del Thing.
2. Mantén un objeto a 6–10 cm y ejecuta `DISTANCE`. Tras un primer acuse, espera un UPDATE periódico (`clase=0`, normalmente cada 30 s con el Shadow sincronizado). Guarda el JSON de `AWS Shadow TX`, su `clientToken` y el acuse de AWS con ese token. Serial debe mostrar `coincide=1 pendiente=1`. Comprueba el valor numérico, `vehiculo_detectado: true` y sus timestamps en el Classic Shadow. Con la distancia estable, el contador `sensor` puede quedarse igual.
3. Repite a 25–30 cm y verifica `vehiculo_detectado: false`. Retira el blanco o cambia su orientación hasta observar `timeout` o `fuera de rango` en Serial. Guarda un UPDATE aceptado con ambos atributos en `null` y verifica que las claves desaparecen. Devuelve el blanco y confirma que reaparecen en un UPDATE aceptado.
4. Ejecuta `SLOT1`, cubre y libera el IR de la plaza 1 y verifica los campos de estacionamiento en el Shadow. Guarda el `STATUS` final. Compara los intentos y acuses observados dentro de la misma sesión por token. Si AWS muestra el acuse y Serial no, registra el caso y revisa la recepción del certificado. Si los campos desaparecen después de un UPDATE aceptado con valores, registra la versión de `update/documents` que los eliminó.
