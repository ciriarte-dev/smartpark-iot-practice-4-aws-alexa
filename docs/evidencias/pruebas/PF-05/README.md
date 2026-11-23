# PF-05 — Inválidos y duplicados del histórico

**Requisitos:** RF11, RNF7. Procedimiento y criterio: [4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales). Análisis: [ficha F6](../../../informe-practica-4.md#f6-integridad-del-almacenamiento-pf-05-y-pnf-05).

**Procedimiento planificado.** Publicar desde el cliente MQTT 4 mensajes inválidos y 1 duplicado en `smartpark/smartpark-grupo9-01/history/v1` y comprobar que no crean ítems.

| Archivo | Qué demuestra |
|---|---|
| [`../../pruebas-automaticas/lambda-unittest.txt`](../../pruebas-automaticas/lambda-unittest.txt) | PA-03 `test_history`: `test_invalid_never_initializes_sdk` (inválidos sin ítem), `test_put_conditional_duplicate_and_retry` (duplicado sin segundo ítem), `test_snapshot_item_and_reduced_capacity` |
| [`../../pruebas-automaticas/host-tests.txt`](../../pruebas-automaticas/host-tests.txt) | PA-02: 12 cargas MQTT del firmware aceptadas por la Lambda; el reintento conserva el `eventKey` |

**Resultado.** No se publicaron inválidos ni duplicados en vivo. Los 2 ítems `prueba-*` de [`../PF-04/historial-muestra.csv`](../PF-04/historial-muestra.csv) son mensajes válidos publicados a mano, no pruebas de descarte.

**Estado:** Verificada con PA-03.
