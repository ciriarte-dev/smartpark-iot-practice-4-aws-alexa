import re

from services.errors import MissingReportedField, UserError
from services.shadow_service import IR_DEBOUNCE_MAX_MS, IR_DEBOUNCE_MIN_MS, PARKING_MODES

from .common import IntentHandler, bound_device, say


def number_slot(handler_input, name, minimum, maximum, error):
    slots = handler_input.request_envelope.request.intent.slots or {}
    value = getattr(slots.get(name), "value", None)
    if not isinstance(value, str) or not re.fullmatch(r"[0-9]{1,4}", value):
        raise UserError(error)
    number = int(value)
    if not minimum <= number <= maximum:
        raise UserError(error)
    return number


def mode_label(mode):
    return "automático" if mode == "automatico" else "manual"


class AvailableSpacesHandler(IntentHandler):
    intent_name = "ConsultarDisponiblesIntent"
    field = "disponibles"
    label = "disponible"

    def handle(self, handler_input):
        shadow, thing = bound_device(handler_input)
        count, capacity = shadow.get_reported_fields(thing, (self.field,), ("capacidad",))
        if type(capacity) is not int or not 1 <= capacity <= 4 or type(count) is not int or not 0 <= count <= capacity:
            raise UserError("El dispositivo no informó un conteo válido de plazas.")
        quantity = f"un espacio {self.label}" if count == 1 else f"{count} espacios {self.label}s"
        return say(handler_input, f"El último reporte indica {quantity} de {capacity}.", keep_open=True)


class OccupiedSpacesHandler(AvailableSpacesHandler):
    intent_name = "ConsultarOcupadosIntent"
    field = "ocupados"
    label = "ocupado"


class ParkingSlotHandler(IntentHandler):
    intent_name = "ConsultarEstacionamientoIntent"

    def handle(self, handler_input):
        shadow, thing = bound_device(handler_input)
        spot = number_slot(handler_input, "espacio", 1, 4, "Dime un número de estacionamiento del uno al cuatro.")
        try:
            value = shadow.get_reported(thing, "slots", str(spot))
        except MissingReportedField:
            capacity = shadow.get_reported(thing, "capacidad")
            if type(capacity) is int and 1 <= capacity < spot:
                raise UserError(f"El último reporte indica que el estacionamiento {spot} no está configurado.") from None
            raise
        if value == "desconocido":
            return say(handler_input, f"El estacionamiento {spot} aún no tiene una lectura confirmada.", keep_open=True)
        if value not in ("libre", "ocupado"):
            raise UserError("El dispositivo no informó un estado válido para ese estacionamiento.")
        return say(handler_input, f"Según el último reporte, el estacionamiento {spot} está {value}.", keep_open=True)


class ParkingModeHandler(IntentHandler):
    intent_name = "ConsultarModoIntent"

    def handle(self, handler_input):
        shadow, thing = bound_device(handler_input)
        value = shadow.get_reported(thing, "modo")
        if value not in PARKING_MODES:
            raise UserError("El dispositivo no informó un modo válido.")
        return say(handler_input, f"El último modo reportado es {mode_label(value)}.", keep_open=True)


class ActivateAutomaticHandler(IntentHandler):
    intent_name = "ActivarModoAutomaticoIntent"
    mode = "automatico"

    def handle(self, handler_input):
        shadow, thing = bound_device(handler_input)
        shadow.set_mode(thing, self.mode)
        return say(handler_input, f"Solicité el modo {mode_label(self.mode)}. El dispositivo aún debe confirmarlo. Puedes consultar el modo del parqueo.", keep_open=True)


class ActivateManualHandler(ActivateAutomaticHandler):
    intent_name = "ActivarModoManualIntent"
    mode = "manual"


class ReadDebounceHandler(IntentHandler):
    intent_name = "ConsultarAntirreboteIntent"

    def handle(self, handler_input):
        shadow, thing = bound_device(handler_input)
        value = shadow.get_reported(thing, "parametros_auto", "antirrebote_ir_ms")
        if type(value) is not int or not IR_DEBOUNCE_MIN_MS <= value <= IR_DEBOUNCE_MAX_MS:
            raise UserError("El dispositivo no informó un valor válido de antirrebote.")
        return say(handler_input, f"El antirrebote reportado es de {value} milisegundos.", keep_open=True)


class SetDebounceHandler(IntentHandler):
    intent_name = "CambiarAntirreboteIntent"

    def handle(self, handler_input):
        shadow, thing = bound_device(handler_input)
        low, high = IR_DEBOUNCE_MIN_MS, IR_DEBOUNCE_MAX_MS
        value = number_slot(handler_input, "milisegundos", low, high,
                            f"Dime un valor entero entre {low} y {high} milisegundos.")
        shadow.set_debounce(thing, value)
        return say(handler_input, f"Solicité {value} milisegundos de antirrebote. El dispositivo aún debe confirmarlo. Puedes consultar el antirrebote.", keep_open=True)
