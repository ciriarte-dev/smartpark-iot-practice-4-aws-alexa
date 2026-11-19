import copy
import json
import unittest
from decimal import Decimal
from pathlib import Path
from unittest.mock import patch

import boto3
from boto3.dynamodb.types import TypeSerializer
from botocore.exceptions import ClientError
from botocore.stub import Stubber

import history_lambda_function as history


class HistoryTest(unittest.TestCase):
    def setUp(self):
        self.event = {
            "thingName": "smartpark-test",
            "schemaVersion": "v1",
            "eventId": "boot123-0001",
            "occurredAt": 1795000000000,
            "receivedAt": 1795000001200,
            "eventType": "sample",
            "origin": "edge",
            "data": {
                "slots": {"1": "libre", "2": "ocupado", "3": "desconocido", "4": "libre"},
                "capacidad": 4, "ocupados": 1, "disponibles": 2, "modo": "manual",
                "distancia_cm": 12.4, "vehiculo_detectado": True,
            },
        }

    def test_snapshot_item_and_reduced_capacity(self):
        item = history.validate_event(self.event)
        self.assertEqual(item["eventKey"], "1795000000000#boot123-0001")
        self.assertEqual(item["data"]["distancia_cm"], Decimal("12.4"))
        reduced = self.event["data"]
        reduced.update(capacidad=2, disponibles=1, distancia_cm=None, vehiculo_detectado=None)
        reduced["slots"] = {"1": "libre", "2": "ocupado"}
        self.assertIsNone(history.validate_event(self.event)["data"]["distancia_cm"])
        reduced.update(slot=2, estado_anterior="libre")
        self.event["eventType"] = "occupancy_changed"
        self.assertEqual(history.validate_event(self.event)["data"]["slot"], 2)
        reduced["estado_anterior"] = "ocupado"
        with self.assertRaises(history.InvalidEvent):
            history.validate_event(self.event)

    def test_barrier_origins_and_actual_transitions(self):
        self.event.update(eventType="barrier_changed", data={
            "anterior": "cerrada", "actual": "abierta", "posicion_estimada": True,
            "requestId": "voice-1",
        })
        for origin in ("voz", "edge", "nube", "legado"):
            with self.subTest(origin=origin):
                self.event["origin"] = origin
                self.assertEqual(history.validate_event(self.event)["origin"], origin)
        self.event["origin"] = "edge"
        self.event["data"].update(anterior="desconocida")
        history.validate_event(self.event)
        for invalid in ({"anterior": "abierta"}, {"posicion_estimada": 1}, {"requestId": "a/b"}):
            event = copy.deepcopy(self.event)
            event["data"].update(invalid)
            with self.subTest(invalid=invalid), self.assertRaises(history.InvalidEvent):
                history.validate_event(event)

    def test_invalid_never_initializes_sdk(self):
        top_level = ({"thingName": "a/b"}, {"eventId": {}}, {"occurredAt": True},
                     {"occurredAt": self.event["receivedAt"] + 300001},
                     {"receivedAt": "1795000000000"}, {"schemaVersion": "v2"},
                     {"eventType": "invalid"}, {"origin": "nube"}, {"data": []})
        data_errors = ({"capacidad": 5}, {"capacidad": True}, {"ocupados": 3},
                       {"disponibles": 4}, {"slots": {"1": "libre", "2": "ocupado"}},
                       {"modo": "auto"}, {"distancia_cm": float("nan")},
                       {"distancia_cm": float("inf")}, {"distancia_cm": True},
                       {"distancia_cm": 401}, {"distancia_cm": None},
                       {"vehiculo_detectado": 1}, {"dato_extra": "ignorable"})
        events = [{**self.event, **patch} for patch in top_level]
        events += [{**self.event, "data": {**self.event["data"], **patch}} for patch in data_errors]
        with patch.object(history, "dynamodb") as sdk, self.assertLogs(history.LOG, level="WARNING"):
            for event in [None, [], *events]:
                with self.subTest(event=event):
                    self.assertEqual(history.lambda_handler(event, None), {"status": "invalid"})
            sdk.assert_not_called()

    def test_put_conditional_duplicate_and_retry(self):
        client = boto3.client("dynamodb", region_name="us-east-1",
                              aws_access_key_id="testing", aws_secret_access_key="testing")
        item = history.validate_event(self.event)
        serializer = TypeSerializer()
        expected = {
            "TableName": "SmartParkHistory",
            "ConditionExpression": "attribute_not_exists(thingName) AND attribute_not_exists(eventKey)",
            "Item": {key: serializer.serialize(value) for key, value in item.items()},
        }
        replay = {**self.event, "receivedAt": self.event["receivedAt"] + 999}
        duplicate = copy.deepcopy(expected)
        duplicate["Item"]["receivedAt"] = {"N": str(replay["receivedAt"])}
        with Stubber(client) as stub, patch.object(history, "dynamodb", return_value=client), \
                patch.dict("os.environ", {"HISTORY_TABLE_NAME": "SmartParkHistory"}):
            stub.add_response("put_item", {}, expected)
            stub.add_client_error("put_item", "ConditionalCheckFailedException", expected_params=duplicate)
            stub.add_client_error("put_item", "ProvisionedThroughputExceededException", expected_params=expected)
            self.assertEqual(history.lambda_handler(self.event, None), {"status": "stored"})
            self.assertEqual(history.lambda_handler(replay, None), {"status": "duplicate"})
            with self.assertRaises(ClientError):
                history.lambda_handler(self.event, None)
            stub.assert_no_pending_responses()

    def test_templates_keep_rule_identity_and_table_keys_aligned(self):
        templates = Path(__file__).resolve().parents[2] / "aws/history"
        rule = json.loads((templates / "iot-rule.example.json").read_text())
        table = json.loads((templates / "table.example.json").read_text())
        grant = json.loads((templates / "iot-invoke-permission.example.json").read_text())
        device = json.loads((templates / "device-publish-policy.example.json").read_text())
        self.assertIn("topic(2) AS thingName, topic(4) AS schemaVersion", rule["sql"])
        self.assertIn("FROM 'smartpark/+/history/v1'", rule["sql"])
        self.assertTrue(rule["ruleDisabled"])
        self.assertTrue(rule["actions"][0]["lambda"]["functionArn"].endswith(grant["FunctionName"]))
        self.assertEqual(grant["SourceAccount"], "<ACCOUNT_ID>")
        self.assertEqual(grant["SourceArn"].split("rule/")[1], "SmartParkHistoryIngest")
        self.assertIn("topic/smartpark/${iot:Connection.Thing.ThingName}/history/v1",
                      device["Statement"][0]["Resource"])
        self.assertEqual(table["KeySchema"], [
            {"AttributeName": "thingName", "KeyType": "HASH"},
            {"AttributeName": "eventKey", "KeyType": "RANGE"},
        ])


if __name__ == "__main__":
    unittest.main()
