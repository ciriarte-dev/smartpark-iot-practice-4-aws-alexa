"""Formato V2 de la orden de barrera (desired.orden_barrera) que escribe la skill.

Las validaciones replican las de AwsIoTManager::applyDesired() en el firmware:
id de 1 a 32 caracteres [A-Za-z0-9_-], origen voz, solicitada_ms creciente y
una vigencia (vence_s * 1000 - solicitada_ms) mayor que 0 y de hasta 30 s.
"""
import json
import re
import unittest
from unittest.mock import MagicMock

from services.shadow_service import BARRIER_ORDER_TTL_S, ShadowService

THING = "parqueo-del-usuario"
FIXED_NOW_S = 1795000000.123
ID_PATTERN = re.compile(r"[A-Za-z0-9_-]{1,32}")


class FixedClock:
    """Reloj inyectable: devuelve el instante actual y avanza solo con advance()."""

    def __init__(self, now):
        self.now = now

    def __call__(self):
        return self.now

    def advance(self, seconds):
        self.now += seconds


class BarrierOrderTests(unittest.TestCase):
    def setUp(self):
        self.clock = FixedClock(FIXED_NOW_S)
        self.data = MagicMock()
        self.data.update_thing_shadow.return_value = {}
        self.service = ShadowService(MagicMock(), lambda: self.data, clock=self.clock)

    def sent_desired(self):
        payload = self.data.update_thing_shadow.call_args.kwargs["payload"]
        return json.loads(payload)["state"]["desired"]

    def test_v2_format_with_voice_origin_and_valid_id(self):
        for state in ("abierta", "cerrada"):
            with self.subTest(state=state):
                self.service.set_barrier(THING, state)
                self.assertEqual(self.data.update_thing_shadow.call_args.kwargs["thingName"], THING)
                desired = self.sent_desired()
                self.assertEqual(set(desired), {"barrera", "orden_barrera"})
                self.assertIsNone(desired["barrera"])
                order = desired["orden_barrera"]
                self.assertEqual(set(order), {"id", "objetivo", "origen", "solicitada_ms", "vence_s"})
                self.assertEqual(order["objetivo"], state)
                self.assertEqual(order["origen"], "voz")
                self.assertIsInstance(order["id"], str)
                self.assertRegex(order["id"], ID_PATTERN)
                self.assertLessEqual(len(order["id"]), 32)

    def test_issued_matches_clock_and_validity_within_ttl(self):
        self.service.set_barrier(THING, "abierta")
        order = self.sent_desired()["orden_barrera"]
        self.assertIs(type(order["solicitada_ms"]), int)
        self.assertIs(type(order["vence_s"]), int)
        self.assertEqual(order["solicitada_ms"], int(FIXED_NOW_S * 1000))
        validity_ms = order["vence_s"] * 1000 - order["solicitada_ms"]
        self.assertGreater(validity_ms, 0)
        self.assertLessEqual(validity_ms, 30_000)
        self.assertLessEqual(validity_ms, BARRIER_ORDER_TTL_S * 1000)

    def test_consecutive_orders_have_distinct_ids_and_increasing_issued_time(self):
        self.service.set_barrier(THING, "abierta")
        first = self.sent_desired()["orden_barrera"]
        self.clock.advance(1)
        self.service.set_barrier(THING, "cerrada")
        second = self.sent_desired()["orden_barrera"]
        self.assertNotEqual(first["id"], second["id"])
        self.assertGreater(second["solicitada_ms"], first["solicitada_ms"])

    def test_invalid_target_raises_without_updating_shadow(self):
        for state in ("abrir", "", None, "ABIERTA"):
            with self.subTest(state=state):
                with self.assertRaises(ValueError):
                    self.service.set_barrier(THING, state)
        self.data.update_thing_shadow.assert_not_called()


if __name__ == "__main__":
    unittest.main()
