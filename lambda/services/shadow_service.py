"""Shadow clasico. Sin polling, sin consultas previas a una orden."""
import json
import math
import time
import uuid

from botocore.exceptions import BotoCoreError, ClientError

from .errors import ServiceError, UserError
from .validation import validate_thing

# Mantener en sincronía con IR_DEBOUNCE_MIN_MS e IR_DEBOUNCE_MAX_MS del firmware.
IR_DEBOUNCE_MIN_MS = 50
IR_DEBOUNCE_MAX_MS = 1000
PARKING_MODES = ("manual", "automatico")
# Mantener en sincronía con AWS_ORDER_TTL_S del firmware (vigencia máxima de una orden V2).
BARRIER_ORDER_TTL_S = 30


class ShadowService:
    def __init__(self, iot_client, data_client_factory, max_age=180, clock=time.time):
        self.iot = iot_client
        self.data_client_factory = data_client_factory
        self.max_age = max_age
        self.clock = clock

    def describe_thing(self, thing):
        validate_thing(thing)
        try:
            self.iot.describe_thing(thingName=thing)
        except ClientError as exc:
            if exc.response.get("Error", {}).get("Code") == "ResourceNotFoundException":
                raise UserError("El dispositivo autorizado ya no existe. Contacta al administrador.") from exc
            raise ServiceError() from exc
        except BotoCoreError as exc:
            raise ServiceError() from exc

    def get_reported(self, thing, field, *nested):
        return self.get_reported_fields(thing, (field, *nested))[0]

    def get_reported_fields(self, thing, *paths):
        validate_thing(thing)
        try:
            result = self.data_client_factory().get_thing_shadow(thingName=thing)
            body = result["payload"]
            try:
                raw = body.read(65537)
            finally:
                body.close()
            if len(raw) > 65536:
                raise ValueError("size")
            def reject_constant(_):
                raise ValueError("nonfinite")
            document = json.loads(raw, parse_constant=reject_constant)
            values = []
            now = self.clock()
            for path in paths:
                value = document["state"]["reported"]
                metadata = document["metadata"]["reported"]
                for key in path:
                    value = value[key]
                    metadata = metadata[key]
                timestamp = metadata["timestamp"]
                if type(timestamp) not in (int, float) or not math.isfinite(timestamp):
                    raise ValueError("timestamp")
                if not -30 <= now - timestamp <= self.max_age:
                    raise UserError("El último dato del dispositivo está desactualizado. No puedo confirmar su estado actual.")
                values.append(value)
            return tuple(values)
        except ClientError as exc:
            if exc.response.get("Error", {}).get("Code") == "ResourceNotFoundException":
                raise UserError("El dispositivo todavía no tiene un Shadow disponible.") from exc
            raise ServiceError() from exc
        except BotoCoreError as exc:
            raise ServiceError() from exc
        except (KeyError, TypeError, ValueError, UnicodeError, AttributeError) as exc:
            raise UserError("No tengo un dato reportado válido y reciente del dispositivo.") from exc

    def set_barrier(self, thing, state):
        if state not in {"abierta", "cerrada"}:
            raise ValueError("Unsupported barrier target")
        issued_ms = int(self.clock() * 1000)
        order = {
            "id": uuid.uuid4().hex,
            "objetivo": state,
            "origen": "voz",
            "solicitada_ms": issued_ms,
            "vence_s": issued_ms // 1000 + BARRIER_ORDER_TTL_S,
        }
        # "barrera": None elimina una orden V1 que pudiera quedar en desired.
        self._update_desired(thing, {"barrera": None, "orden_barrera": order})

    def set_mode(self, thing, mode):
        if mode not in PARKING_MODES:
            raise ValueError("Unsupported parking mode")
        self._update_desired(thing, {"modo": mode})

    def set_debounce(self, thing, milliseconds):
        if type(milliseconds) is not int or not IR_DEBOUNCE_MIN_MS <= milliseconds <= IR_DEBOUNCE_MAX_MS:
            raise ValueError("Unsupported IR debounce")
        self._update_desired(thing, {"parametros_auto": {"antirrebote_ir_ms": milliseconds}})

    def _update_desired(self, thing, patch):
        validate_thing(thing)
        payload = json.dumps({"state": {"desired": patch}}, separators=(",", ":")).encode()
        try:
            response = self.data_client_factory().update_thing_shadow(thingName=thing, payload=payload)
            body = response.get("payload")
            if body is not None:
                body.close()
        except (BotoCoreError, ClientError) as exc:
            raise ServiceError() from exc
