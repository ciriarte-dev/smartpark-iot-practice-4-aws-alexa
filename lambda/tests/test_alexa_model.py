"""Valida el modelo de interacción desplegado en la consola de Alexa.

Modelo: alexa/es-US.smartpark-v2.final.json. Los handlers se localizan leyendo
los atributos intent_name de lambda/handlers/ con ast, sin importar el SDK.
"""
import ast
import json
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MODEL_PATH = ROOT / "alexa" / "es-US.smartpark-v2.final.json"
HANDLERS_DIR = ROOT / "lambda" / "handlers"


def load_language_model():
    with MODEL_PATH.open(encoding="utf-8") as model_file:
        return json.load(model_file)["interactionModel"]["languageModel"]


def handled_intents():
    names = set()
    for path in sorted(HANDLERS_DIR.glob("*.py")):
        tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        for node in ast.walk(tree):
            if isinstance(node, ast.Assign) and isinstance(node.value, ast.Constant):
                targets = {target.id for target in node.targets if isinstance(target, ast.Name)}
                if "intent_name" in targets and isinstance(node.value.value, str):
                    names.add(node.value.value)
    return names


class AlexaFinalModelTests(unittest.TestCase):
    def setUp(self):
        self.model = load_language_model()
        self.intents = self.model["intents"]

    def test_invocation_name(self):
        self.assertEqual(self.model["invocationName"], "parqueo inteligente")

    def test_every_custom_intent_has_a_handler(self):
        custom = [intent["name"] for intent in self.intents if not intent["name"].startswith("AMAZON.")]
        self.assertEqual(len(custom), 16)
        self.assertEqual(len(set(custom)), 16)
        missing = set(custom) - handled_intents()
        self.assertEqual(missing, set(), f"Intents sin handler en lambda/handlers/: {sorted(missing)}")

    def test_samples_are_unique_and_use_declared_slots(self):
        seen = {}
        for intent in self.intents:
            declared = {slot["name"] for slot in intent.get("slots", [])}
            for sample in intent.get("samples", []):
                normalized = " ".join(sample.casefold().split())
                self.assertNotIn(normalized, seen, f"Frase duplicada en {intent['name']} y {seen.get(normalized)}")
                seen[normalized] = intent["name"]
                used = set(re.findall(r"{([^}]+)}", sample))
                self.assertLessEqual(used, declared, f"{intent['name']}: slot no declarado en '{sample}'")


if __name__ == "__main__":
    unittest.main()
