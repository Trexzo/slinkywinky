"""R262 current-PR2 source catalog parity negative regressions."""
import importlib.util
import os
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools" / "audit_r262_cross_branch_catalog.py"
spec = importlib.util.spec_from_file_location("r262_catalog", SCRIPT)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

class CrossBranchCatalogTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        pr2 = os.environ.get("SLINKYWINKY_R262_PR2_ROOT")
        if not pr2:
            raise RuntimeError("R262 requires explicit current-PR2 source checkout")
        cls.gui = (ROOT / "src" / "r115_recovered_catalog.inc").read_text(encoding="utf-8")
        cls.matrix = (Path(pr2) / "docs" / "MODULE_MATRIX.md").read_text(encoding="utf-8")
        cls.headers = audit.read_headers(pr2)

    def test_exact_name_category_and_source_coverage(self):
        result = audit.audit(self.gui, self.matrix, self.headers)
        self.assertEqual((result["gui_descriptors"], result["native_registered"],
                          result["partial_controller_headers"],
                          result["experimental_models"]), (67, 66, 60, 6))

    def test_reject_ghost_hand_gui_category_drift(self):
        old = '{"Ghost Hand",MenuTab::Block}'
        self.assertEqual(self.gui.count(old), 1)
        changed = self.gui.replace(old, '{"Ghost Hand",MenuTab::Combat}')
        with self.assertRaisesRegex(ValueError, "category count drift"):
            audit.audit(changed, self.matrix, self.headers)

    def test_reject_registered_name_drift(self):
        old = '{"Ghost Hand",MenuTab::Block}'
        changed = self.gui.replace(old, '{"Ghost Handling",MenuTab::Block}')
        with self.assertRaisesRegex(ValueError, "descriptor names differ"):
            audit.audit(changed, self.matrix, self.headers)

    def test_reject_pr2_native_category_drift(self):
        old = '| block | `ghost_hand` |'
        self.assertEqual(self.matrix.count(old), 1)
        changed = self.matrix.replace(old, '| combat | `ghost_hand` |')
        with self.assertRaisesRegex(ValueError, "category drift"):
            audit.audit(self.gui, changed, self.headers)

    def test_reject_missing_controller_header(self):
        self.assertIn("ghost_hand", self.headers)
        with self.assertRaisesRegex(ValueError, "header inventory drift"):
            audit.audit(self.gui, self.matrix, self.headers - {"ghost_hand"})

    def test_reject_descriptor_only_promotion(self):
        self.assertIn("| utility | `chest_stealer` | Not implemented |", self.matrix)
        changed = self.matrix.replace(
            "| utility | `chest_stealer` | Not implemented |",
            "| utility | `chest_stealer` | Experimental decision model (offline) |")
        with self.assertRaisesRegex(ValueError, "Chest Stealer"):
            audit.audit(self.gui, changed, self.headers)

if __name__ == "__main__":
    unittest.main()
