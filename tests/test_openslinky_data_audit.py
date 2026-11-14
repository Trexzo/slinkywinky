#!/usr/bin/env python3
"""R173: synthetic COFF corruption and pinned public data object checks."""
import importlib.util
import struct
import tempfile
import unittest
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "openslinky_data_audit", ROOT / "tools/audit_openslinky_data.py")
audit_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit_module)


class TestOpenSlinkyDataAudit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.obj = ROOT / "reference_data.obj"
        cls.ref = ROOT / "references/OpenSlinky"

    def test_truncated_coff_rejected(self):
        with self.assertRaisesRegex(ValueError, "truncated"):
            audit_module.COFF(b"not COFF")

    def test_wrong_machine_rejected(self):
        binary = bytearray(20)
        struct.pack_into("<HHIIIHH", binary, 0, 0x014C, 1, 0, 0, 0, 0, 0)
        with self.assertRaisesRegex(ValueError, "Windows x64"):
            audit_module.COFF(binary)

    def test_pinned_object_public_source_inventory(self):
        if not self.obj.is_file():
            self.skipTest("Assembled source-only COFF fixture absent")
        result = audit_module.audit(self.ref, self.obj)
        self.assertEqual(result["compiled_data_bytes"], 136840)
        self.assertEqual(result["original_candidate_data_bytes_expected"], 136712)
        self.assertEqual(result["appended_source_data_bytes"], 128)
        self.assertEqual(result["pointer_relocations_in_candidate_range"], 2438)
        self.assertEqual(result["pointer_relocations_in_appended_range"], 4)
        self.assertIsNone(result["original_pointer_targets_verified"])
        self.assertFalse(result["private_candidate_tested"])

    def test_relocation_kind_mutation_rejected(self):
        if not self.obj.is_file():
            self.skipTest("Assembled source-only COFF fixture absent")
        binary = bytearray(self.obj.read_bytes())
        # The data section is the second section in the pinned source COFF.
        section_header = 20 + 40
        reloc_offset = struct.unpack_from("<I", binary, section_header + 24)[0]
        struct.pack_into("<H", binary, reloc_offset + 8, 2)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "mutated.obj"
            path.write_bytes(binary)
            with self.assertRaisesRegex(ValueError, "Unexpected non-absolute"):
                audit_module.audit(self.ref, path)

    def test_fake_private_data_zip_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "fake.zip"
            with zipfile.ZipFile(path, "w") as zf:
                zf.writestr("local/slinky2.decrypted/data.bin", b"\0" * 128)
            with self.assertRaisesRegex(ValueError, "differs from pinned build"):
                audit_module.original_candidate_data(path)

    def test_duplicate_private_data_entries_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "ambiguous.zip"
            with zipfile.ZipFile(path, "w") as zf:
                zf.writestr("a/slinky2.decrypted/data.bin", b"a")
                zf.writestr("b/slinky2.decrypted/data.bin", b"b")
            with self.assertRaisesRegex(ValueError, "found 2"):
                audit_module.original_candidate_data(path)


if __name__ == "__main__":
    unittest.main()
