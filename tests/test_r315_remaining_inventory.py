import copy
import importlib.util
import json
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("audit_r315",ROOT/"tools/audit_r315_remaining_inventory.py")
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

class InventoryTests(unittest.TestCase):
    def setUp(self):
        self.ledger=json.loads(mod.LEDGER.read_text(encoding="utf-8"))
        self.matrix=(ROOT/"docs/MODULE_MATRIX.md").read_text(encoding="utf-8")
    def test_current_accounting(self):
        r=mod.audit(self.ledger,self.matrix)
        self.assertEqual(sum(r["coverage"].values()),66)
        self.assertFalse(r["archive_verified_this_run"])
        self.assertFalse(r["runtime_parity_verified"])
        self.assertNotIn("chest_stealer",r["missing"])
    def test_category_drift(self):
        with self.assertRaises(ValueError):
            mod.audit(self.ledger,self.matrix.replace("| combat | \x60backtrack\x60","| move | \x60backtrack\x60"))
    def test_count_drift(self):
        self.ledger["modules"][0]["settings_reported"]+=1
        with self.assertRaises(ValueError):
            mod.audit(self.ledger,self.matrix)
    def test_duplicate(self):
        self.ledger["modules"][1]=copy.deepcopy(self.ledger["modules"][0])
        with self.assertRaises(ValueError):
            mod.audit(self.ledger,self.matrix)
    def test_reference_hash(self):
        self.ledger["modules"][0]["sha256"]="bad"
        with self.assertRaises(ValueError):
            mod.audit(self.ledger,self.matrix)
