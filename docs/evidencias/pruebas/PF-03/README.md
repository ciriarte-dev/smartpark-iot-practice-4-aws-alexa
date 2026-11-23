# PF-03 — Vinculación usuario ↔ Thing

**Requisitos:** RF7, RF8, RF9. Procedimiento y criterio: [4.2 del informe](../../../informe-practica-4.md#42-pruebas-funcionales). Análisis: [ficha F10](../../../informe-practica-4.md#f10-vinculación-pf-03).

**Procedimiento planificado.** Generar un PIN con `SmartParkPairingAdmin`, vincular, cambiar la vinculación, desvincular, probar un PIN vencido y un Thing inexistente.

| Archivo | Qué demuestra |
|---|---|
| [`../../alexa/test-publica.png`](../../alexa/test-publica.png) | En el simulador: *"desvincular parqueo"* → *"Desvinculé tu parqueo."*; sin vínculo, la guía para pedir un código; vinculación con un código de 4 dígitos (oculto) → *"Tu parqueo quedó vinculado…"* |
| [`../../pruebas-automaticas/lambda-unittest.txt`](../../pruebas-automaticas/lambda-unittest.txt) | PA-03: `test_admin`, `test_pin_public`, `test_isolation`, `test_existing_thing_required`, `test_thing_not_found`, `test_twelve_digit_pin_is_rejected_by_both_binding_intents_without_aws` |

**Resultado.** Desvinculación, guía sin vínculo y vinculación verificadas en vivo; la captura no tiene fecha y su mensaje de bienvenida difiere del código actual ([`system_handlers.py#L20`](../../../../lambda/handlers/system_handlers.py#L20)), así que corresponde a un despliegue anterior del backend. Cambio de vinculación, PIN de 300 s con 3 intentos y Thing inexistente: solo PA-03 (108/108 OK).

**Estado:** ⚠️ Cumple parcial (RF7, RF8, RF9).
