#!/usr/bin/env python3
"""R172 synthetic-only tests for pinned native-dispatch source inspection."""
import importlib.util
import tempfile
import unittest
import zipfile
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "tools/audit_openslinky_dispatch.py"
spec = importlib.util.spec_from_file_location("openslinky_dispatch_audit", SCRIPT)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class TestOpenSlinkyDispatch(unittest.TestCase):
    def test_pinned_public_source_contract(self):
        root = Path(__file__).resolve().parents[1] / "references/OpenSlinky"
        if not (root / "src/engine/jni/module_manager.cpp").exists():
            self.skipTest("Pinned upstream submodule not checked out")
        result = audit.source_audit(root)
        self.assertEqual(result["source_event_slot_count"], 27)
        self.assertTrue(result["source_dispatch_has_out_of_range_guard"])
        self.assertTrue(result["source_is_handled_has_out_of_range_guard"])
        self.assertTrue(result["source_toggle_queue_has_capacity_guard"])
        self.assertTrue(result["source_toggle_processing_checks_world_and_player"])
        self.assertTrue(result["source_manager_registers_67"])

    def test_no_functions_cannot_pass(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "src/engine/jni/module_manager.cpp"
            path.parent.mkdir(parents=True)
            path.write_text('extern "C" void dispatch_event() {}', encoding="utf-8")
            with self.assertRaises(ValueError):
                audit.source_audit(Path(tmp))

    def test_balanced_function_body(self):
        source = 'extern "C" void dispatch_event(Mgr* m) { if (true) { work(); } }\nextern "C" void after() { wrong(); }'
        body = audit.function_body(source, "dispatch_event")
        self.assertIn("work()", body)
        self.assertNotIn("wrong()", body)

    def test_ambiguous_function_rejected(self):
        source = 'extern "C" void dispatch_event() {}\nextern "C" void dispatch_event(int x) {}'
        with self.assertRaises(ValueError):
            audit.function_body(source, "dispatch_event")

    def test_synthetic_private_input_rejected_before_instruction_analysis(self):
        with tempfile.TemporaryDirectory() as tmp:
            archive = Path(tmp) / "fake.zip"
            with zipfile.ZipFile(archive, "w") as zf:
                zf.writestr("fake/slinky2.decrypted/text.bin", b"\x00" * 100)
            with self.assertRaisesRegex(ValueError, "hash mismatches"):
                audit.private_native_audit(archive)

    def test_ambiguous_zip_members_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            archive = Path(tmp) / "fake.zip"
            with zipfile.ZipFile(archive, "w") as zf:
                zf.writestr("one/slinky2.decrypted/text.bin", b"A")
                zf.writestr("two/slinky2.decrypted/text.bin", b"B")
            with self.assertRaisesRegex(ValueError, "found 2"):
                audit.private_native_audit(archive)

    def test_native_windows_have_ordered_nonoverlapping_ranges(self):
        ranges = sorted((start, end) for start, end, _ in audit.NATIVE_WINDOWS.values())
        self.assertEqual(len(ranges), 5)
        self.assertTrue(all(start >= audit.CANDIDATE_TEXT_RVA and end > start for start, end in ranges))
        self.assertTrue(all(a_end <= b_start for (_, a_end), (b_start, _) in zip(ranges, ranges[1:])))


if __name__ == "__main__":
    unittest.main()
