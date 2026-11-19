"""SmartParkHistoryProcessor, invoked asynchronously by the SmartPark IoT rule."""
import logging
import math
import os
import re
from decimal import Decimal
from functools import lru_cache

import boto3
from boto3.dynamodb.types import TypeSerializer
from botocore.exceptions import ClientError

LOG = logging.getLogger(__name__)
THING_NAME = re.compile(r"[A-Za-z0-9:_-]{1,128}")
EVENT_ID = re.compile(r"[A-Za-z0-9_-]{1,64}")
SLOT_STATES = ("libre", "ocupado", "desconocido")
BARRIER_STATES = ("abierta", "cerrada")
MIN_EPOCH_MS = 946684800000
MAX_EPOCH_MS = 9999999999999
CLOCK_SKEW_MS = 300_000


class InvalidEvent(ValueError):
    """Permanent schema or measurement error; no asynchronous retry needed."""


def integer(value, minimum, maximum, field):
    if type(value) is not int or not minimum <= value <= maximum:
        raise InvalidEvent(field)
    return value


def matching(value, pattern, field):
    if not isinstance(value, str) or not pattern.fullmatch(value):
        raise InvalidEvent(field)
    return value


def validate_snapshot(data, *, transition):
    keys = {"slots", "capacidad", "ocupados", "disponibles", "modo",
            "distancia_cm", "vehiculo_detectado"}
    if transition:
        keys |= {"slot", "estado_anterior"}
    if data.keys() != keys:
        raise InvalidEvent("campos de lectura")
    capacity = integer(data["capacidad"], 1, 4, "capacidad")
    slots = data["slots"]
    if (not isinstance(slots, dict) or slots.keys() != {str(i) for i in range(1, capacity + 1)}
            or any(state not in SLOT_STATES for state in slots.values())):
        raise InvalidEvent("slots")
    occupied = integer(data["ocupados"], 0, capacity, "ocupados")
    available = integer(data["disponibles"], 0, capacity, "disponibles")
    if occupied != sum(value == "ocupado" for value in slots.values()):
        raise InvalidEvent("conteo ocupados")
    if available != sum(value == "libre" for value in slots.values()):
        raise InvalidEvent("conteo disponibles")
    if data["modo"] not in ("manual", "automatico"):
        raise InvalidEvent("modo")
    distance = data["distancia_cm"]
    if distance is not None:
        if type(distance) not in (int, float) or not 2 <= distance <= 400 or not math.isfinite(distance):
            raise InvalidEvent("distancia_cm")
    presence = data["vehiculo_detectado"]
    if presence is not None and (type(presence) is not bool or distance is None):
        raise InvalidEvent("vehiculo_detectado")
    if transition:
        slot = integer(data["slot"], 1, capacity, "slot")
        previous = data["estado_anterior"]
        if previous not in SLOT_STATES or previous == slots[str(slot)]:
            raise InvalidEvent("transición de plaza")
    return {**data, "distancia_cm": Decimal(str(distance)) if distance is not None else None}


def validate_barrier(data):
    if data.keys() not in ({"anterior", "actual", "posicion_estimada"},
                           {"anterior", "actual", "posicion_estimada", "requestId"}):
        raise InvalidEvent("campos de barrera")
    previous, current = data["anterior"], data["actual"]
    if (previous not in (*BARRIER_STATES, "desconocida") or current not in BARRIER_STATES
            or previous == current or data["posicion_estimada"] is not True):
        raise InvalidEvent("transición de barrera")
    if "requestId" in data:
        matching(data["requestId"], EVENT_ID, "requestId")
    return data


def validate_event(event):
    if not isinstance(event, dict) or event.get("schemaVersion") != "v1":
        raise InvalidEvent("schemaVersion")
    thing = matching(event.get("thingName"), THING_NAME, "thingName")
    event_id = matching(event.get("eventId"), EVENT_ID, "eventId")
    received = integer(event.get("receivedAt"), MIN_EPOCH_MS, MAX_EPOCH_MS, "receivedAt")
    occurred = integer(event.get("occurredAt"), MIN_EPOCH_MS,
                       min(MAX_EPOCH_MS, received + CLOCK_SKEW_MS), "occurredAt")
    data = event.get("data")
    if not isinstance(data, dict):
        raise InvalidEvent("data")
    event_type, origin = event.get("eventType"), event.get("origin")
    if event_type == "barrier_changed" and origin in ("voz", "edge", "nube", "legado"):
        data = validate_barrier(data)
    elif event_type in ("sample", "occupancy_changed") and origin == "edge":
        data = validate_snapshot(data, transition=event_type == "occupancy_changed")
    else:
        raise InvalidEvent("tipo de evento u origen")
    return {"thingName": thing, "eventKey": f"{occurred:013d}#{event_id}",
            "schemaVersion": "v1", "eventId": event_id, "occurredAt": occurred,
            "receivedAt": received, "eventType": event_type, "origin": origin, "data": data}


@lru_cache(maxsize=1)
def dynamodb():
    return boto3.client("dynamodb")


def lambda_handler(event, _context):
    try:
        item = validate_event(event)
    except InvalidEvent as exc:
        LOG.warning("history_invalid field=%s", exc)
        return {"status": "invalid"}
    serializer = TypeSerializer()
    try:
        dynamodb().put_item(
            TableName=os.environ["HISTORY_TABLE_NAME"],
            Item={key: serializer.serialize(value) for key, value in item.items()},
            ConditionExpression="attribute_not_exists(thingName) AND attribute_not_exists(eventKey)",
        )
    except ClientError as exc:
        if exc.response["Error"]["Code"] == "ConditionalCheckFailedException":
            return {"status": "duplicate"}
        raise
    return {"status": "stored"}
