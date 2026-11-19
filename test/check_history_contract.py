"""Validate payloads emitted by the C++ host test with the Lambda's real validator."""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lambda"))
from history_lambda_function import validate_event


def main():
    payloads = [json.loads(line) for line in Path(sys.argv[1]).read_text().splitlines()]
    fields = {"eventId", "occurredAt", "eventType", "origin", "data"}
    event_types, barrier_origins, keys = set(), set(), set()
    for payload in payloads:
        assert payload.keys() == fields
        # The IoT Rule derives these fields from the topic and the ingestion time.
        event = {**payload, "thingName": "smartpark-grupo9-01", "schemaVersion": "v1",
                 "receivedAt": payload["occurredAt"] + len(keys)}
        item = validate_event(event)
        keys.add((item["thingName"], item["eventKey"]))
        event_types.add(item["eventType"])
        if item["eventType"] == "barrier_changed":
            barrier_origins.add(item["origin"])
    assert event_types == {"barrier_changed", "occupancy_changed", "sample"}
    assert barrier_origins == {"voz", "edge", "nube"}
    assert payloads[0] == payloads[1] and len(keys) == len(payloads) - 1
    print(f"PASS: {len(payloads)} cargas MQTT del firmware aceptadas por la Lambda; reintento conserva la clave.")


if __name__ == "__main__":
    main()
