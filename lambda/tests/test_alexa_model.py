"""Valida el modelo de interacción local preparado para Alexa.

Modelo: alexa/es-US.smartpark-v2.final.json. Los handlers se localizan leyendo
los atributos intent_name de lambda/handlers/ con ast, sin importar el SDK.
"""
import ast
import json
import re
import unittest
import unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MODEL_PATH = ROOT / "alexa" / "es-US.smartpark-v2.final.json"
HANDLERS_DIR = ROOT / "lambda" / "handlers"


def load_language_model():
    with MODEL_PATH.open(encoding="utf-8") as model_file:
        return json.load(model_file)["interactionModel"]["languageModel"]


def handled_intents():
    entry = ast.parse((ROOT / "lambda" / "lambda_function.py").read_text())
    registered = {
        elt.func.id
        for loop in entry.body if isinstance(loop, ast.For) and isinstance(loop.iter, ast.Tuple)
        for elt in loop.iter.elts if isinstance(elt, ast.Call) and isinstance(elt.func, ast.Name)
    }
    names = set()
    for path in sorted(HANDLERS_DIR.glob("*.py")):
        tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        for cls in tree.body:
            if not isinstance(cls, ast.ClassDef) or cls.name not in registered:
                continue
            for node in cls.body:
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
                normalized = " ".join("".join(c for c in unicodedata.normalize("NFD", sample.casefold())
                                              if not unicodedata.combining(c)).split())
                self.assertNotIn(normalized, seen, f"Frase duplicada en {intent['name']} y {seen.get(normalized)}")
                seen[normalized] = intent["name"]
                used = set(re.findall(r"{([^}]+)}", sample))
                self.assertLessEqual(used, declared, f"{intent['name']}: slot no declarado en '{sample}'")

    def test_binding_and_explicit_voice_samples(self):
        intents = {entry["name"]: entry for entry in self.intents}
        for name in ("VincularParqueoIntent", "CambiarVinculacionIntent"):
            self.assertEqual(intents[name]["slots"], [{"name": "codigo", "type": "AMAZON.FOUR_DIGIT_NUMBER"}])
        self.assertIn("consultar estado", intents["EstadoBarreraIntent"]["samples"])
        unlink = intents["DesvincularParqueoIntent"]["samples"]
        self.assertNotIn("desconecta mi dispositivo", unlink)
        self.assertIn("elimina la vinculación de mi parqueo inteligente", unlink)


if __name__ == "__main__":
    unittest.main()
