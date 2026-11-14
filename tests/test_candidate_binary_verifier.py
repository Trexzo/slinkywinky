#!/usr/bin/env python3
"""Synthetic-only tests for the optional local decrypted-candidate verifier."""
import importlib.util
import io
import struct
import tempfile
import unittest
import zipfile
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "tools/verify_candidate_binary.py"
spec = importlib.util.spec_from_file_location("candidate_verifier", SCRIPT)
verifier = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verifier)


def minimal_pe():
    """Small generated PE header and two synthetic sections, not commercial code."""
    data = bytearray(0x600)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", data, 0x86, 2)
    struct.pack_into("<H", data, 0x94, 0)
    for idx, (name, rva, raw, offset) in enumerate((
        (b".text", 0x1000, b"abcd", 0x200),
        (b".rdata", 0x2000, b"WXYZ", 0x300),
    )):
        base = 0x98 + idx * 40
        data[base:base+8] = name.ljust(8, b"\0")
        struct.pack_into("<IIII", data, base+8, len(raw), rva, len(raw), offset)
        data[offset:offset+len(raw)] = raw
    return bytes(data)


class TestLocalCandidateVerifier(unittest.TestCase):
    def test_generated_pe_sections(self):
        sections = verifier.pe_sections(minimal_pe())
        self.assertEqual(sections[".text"], (4, 0x1000, b"abcd"))
        self.assertEqual(sections[".rdata"], (4, 0x2000, b"WXYZ"))

    def test_bad_pe_rejected(self):
        with self.assertRaises(ValueError):
            verifier.pe_sections(b"not a PE")

    def test_section_out_of_bounds_rejected(self):
        pe = bytearray(minimal_pe())
        struct.pack_into("<I", pe, 0x98 + 20, len(pe) + 100)
        with self.assertRaisesRegex(ValueError, "exceeds"):
            verifier.pe_sections(pe)

    def test_member_rejects_ambiguous_reference(self):
        raw = io.BytesIO()
        with zipfile.ZipFile(raw, "w") as z:
            z.writestr("one/slinky2.decrypted/text.bin", b"A")
            z.writestr("two/slinky2.decrypted/text.bin", b"B")
        raw.seek(0)
        with zipfile.ZipFile(raw) as z:
            with self.assertRaisesRegex(ValueError, "found 2"):
                verifier.member(z, "/slinky2.decrypted/text.bin")

    def test_module_manager_source_parse(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "src/engine/jni/module_manager.cpp"
            path.parent.mkdir(parents=True)
            path.write_text(
                "extern int ptr_to_aim_assist_module_descriptor[0x43];\n"
                "manager->count = 0x43;\n"
                "libc_memcpy_alt(manager->mods, ptr_to_aim_assist_module_descriptor, 0x218);\n",
                encoding="utf-8",
            )
            parsed = verifier.inspect_source(Path(tmp))
            self.assertEqual(parsed["source_manager_registered"], 67)
            self.assertEqual(parsed["source_manager_descriptor_copy_bytes"], 536)
            self.assertEqual(parsed["source_manager_table_entries"], 67)


    def test_single_extra_descriptor_found(self):
        candidate = [11, 22, 33, 44]
        candidate_with_chest = [11, 22, 99, 33, 44]
        self.assertEqual(verifier.descriptor_insertion_indices(candidate, candidate_with_chest), [2])

    def test_descriptor_reordering_rejected(self):
        with self.assertRaises(ValueError):
            verifier.descriptor_insertion_indices([11, 22, 33], [33, 22, 11])

    def test_pinned_source_descriptor_table(self):
        root = Path(__file__).resolve().parents[1] / "references/OpenSlinky"
        if not (root / "src/data/rdata.s").is_file():
            self.skipTest("Pinned submodule not available in standalone fixture")
        entries = verifier.mapped_descriptor_table(root)
        self.assertEqual(len(entries), 67)
        self.assertEqual(entries[17][0], "chest_stealer_module_descriptor")
        self.assertEqual(entries[17][1], 0x1802F5288)


if __name__ == "__main__":
    unittest.main()
