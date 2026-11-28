"""R373 source-only positive/negative native category admission tests."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools" / "audit_bridge_native_category.py"
spec = importlib.util.spec_from_file_location("r373_audit", SCRIPT)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

class NativeBinderCategoryAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.matrix = (ROOT / "docs" / "MODULE_MATRIX.md").read_text(encoding="utf-8")
        cls.source = (ROOT / "reference_compat" / "include" / "slinkywinky" /
                      "module_execution_bridge.hpp").read_text(encoding="utf-8")

    def test_authentic_repository_native_categories(self):
        report = audit.audit_sources(self.matrix, self.source)
        self.assertEqual(report["registered"], 66)
        self.assertEqual(report["native_direct_binders"], 46)
        self.assertEqual(report["ghost_hand_native_category"], 3)

    def test_reject_legacy_java_ghost_hand_combat(self):
        old = 'definitions_[id].name!="Ghost Hand" ||\n           definitions_[id].category!=3 ||'
        self.assertEqual(self.source.count(old), 1)
        altered = self.source.replace(old, old.replace("category!=3", "category!=0"))
        with self.assertRaisesRegex(ValueError, "bind_ghost_hand"):
            audit.audit_sources(self.matrix, altered)

    def test_reject_misclassified_source_packet_controller(self):
        old = 'matches_source_row(id,"Backtrack",0'
        self.assertEqual(self.source.count(old) > 0, True)
        altered = self.source.replace(old, 'matches_source_row(id,"Backtrack",4')
        with self.assertRaisesRegex(ValueError, "bind_backtrack"):
            audit.audit_sources(self.matrix, altered)

    def test_reject_missing_category_gate(self):
        old = 'definitions_[id].name!="Ghost Hand" ||\n           definitions_[id].category!=3 ||'
        self.assertEqual(self.source.count(old), 1)
        altered = self.source.replace(old, 'definitions_[id].name!="Ghost Hand" ||')
        with self.assertRaisesRegex(ValueError, "bind_ghost_hand"):
            audit.audit_sources(self.matrix, altered)

    def test_reject_matrix_ghost_hand_wrong_category(self):
        old = '| block | `ghost_hand` |'
        self.assertEqual(self.matrix.count(old), 1)
        altered = self.matrix.replace(old, '| combat | `ghost_hand` |')
        with self.assertRaisesRegex(ValueError, "category drift"):
            audit.audit_sources(altered, self.source)

if __name__ == "__main__":
    unittest.main()
