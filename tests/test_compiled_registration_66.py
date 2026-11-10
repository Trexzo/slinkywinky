"""R194 synthetic verifier tests: no third-party DLL or original byte tables."""
import importlib.util
from pathlib import Path
import struct
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "verify_compiled_registration_66.py"
sys.path.insert(0, str(SCRIPT.parent))
spec = importlib.util.spec_from_file_location("compiled_registration", SCRIPT)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def invented_names():
    names = ["synthetic_descriptor_" + str(i) for i in range(67)]
    names[17] = "chest_stealer_module_descriptor"
    return names


def invented_pe_and_map(names=None):
    if names is None:
        names = invented_names()
    base = 0x180000000
    blob = bytearray(0x800)
    blob[:2] = b"MZ"
    struct.pack_into("<I", blob, 0x3C, 0x80)
    blob[0x80:0x84] = b"PE\x00\x00"
    struct.pack_into("<HH", blob, 0x84, 0x8664, 1)
    struct.pack_into("<H", blob, 0x94, 0xF0)
    blob[0x98:0x9A] = b"\x0b\x02"
    struct.pack_into("<Q", blob, 0x98 + 24, base)
    section = 0x98 + 0xF0
    blob[section:section + 8] = b".rdata\x00\x00"
    struct.pack_into("<IIII", blob, section + 8, 0x400, 0x1000, 0x400, 0x200)
    symbols = {audit.TABLE_SYMBOL: base + 0x1000}
    for index, name in enumerate(names):
        symbols[name] = base + 0x3000 + index * 0x10
        struct.pack_into("<Q", blob, 0x200 + index * 8, symbols[name])
    maptext = "\n".join(
        " 0002:00000000       " + name + " " + format(address, "016x") + "     synthetic.obj"
        for name, address in symbols.items()
    ) + "\n"
    return blob, maptext


def sample_disassembly(corrected=True):
    count = "42" if corrected else "43"
    words = [
        "00000000000009c0 <create_module_manager>:",
        "     9ed: 48 c7 47 08 42 00 00 00     movq $0x" + count + ", 0x8(%rdi)",
        "     9f5: b9 42 00 00 00            movl $0x" + count + ", %ecx",
    ]
    if corrected:
        words.extend([
            "     a23: 41 b8 88 00 00 00       movl $0x88, %r8d",
            "     a34: 48 8d 15 90 00 00 00    leaq 0x90(%rip), %rdx",
            "     a3b: 41 b8 88 01 00 00       movl $0x188, %r8d",
            "     a54: 48 83 f9 42             cmpq $0x42, %rcx",
        ])
    else:
        words.append("     a1f: 41 b8 18 02 00 00       movl $0x218, %r8d")
    words.extend([
        "0000000000000de0 <queue_module_toggle>:",
        "     e0b: 49 83 f8 41             cmpq $0x" + ("41" if corrected else "42") + ", %r8",
        "     e8b: 49 83 f8 41             cmpq $0x" + ("41" if corrected else "42") + ", %r8",
    ])
    return "\n".join(words) + "\n"


class LinkedRegistrationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.folder = Path(self.temp.name)
        self.names = invented_names()
        self.blob, self.text = invented_pe_and_map(self.names)
        self.binary = self.folder / "fake.dll"
        self.link_map = self.folder / "fake.map"
        self.binary.write_bytes(self.blob)
        self.link_map.write_text(self.text, encoding="utf-8")

    def test_exact_linked_pointer_matches_synthetic_map(self):
        result = audit.audit_linked_pointer_table(self.binary, self.link_map, self.names)
        self.assertTrue(result["all_linked_descriptor_addresses_match_map"])
        self.assertEqual(result["linked_static_entries"], 67)
        self.assertEqual(result["model_compacted_registration_count"], 66)
        self.assertEqual(result["table_bytes_verified"], 67 * 8)

    def test_pointer_swap_rejected(self):
        changed = bytearray(self.blob)
        a = changed[0x200:0x208]
        changed[0x200:0x208] = changed[0x208:0x210]
        changed[0x208:0x210] = a
        self.binary.write_bytes(changed)
        with self.assertRaisesRegex(ValueError, "index 0"):
            audit.audit_linked_pointer_table(self.binary, self.link_map, self.names)

    def test_missing_descriptor_symbol_rejected(self):
        self.link_map.write_text(
            "\n".join(line for line in self.text.splitlines() if "synthetic_descriptor_1 " not in line),
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "symbol missing"):
            audit.audit_linked_pointer_table(self.binary, self.link_map, self.names)

    def test_wrong_machine_rejected(self):
        changed = bytearray(self.blob)
        struct.pack_into("<H", changed, 0x84, 0x014C)
        self.binary.write_bytes(changed)
        with self.assertRaisesRegex(ValueError, "AMD64"):
            audit.audit_linked_pointer_table(self.binary, self.link_map, self.names)

    def test_wrong_section_rejected(self):
        changed = bytearray(self.blob)
        changed[0x188:0x190] = b".data\x00\x00\x00"
        self.binary.write_bytes(changed)
        with self.assertRaisesRegex(ValueError, "rdata"):
            audit.audit_linked_pointer_table(self.binary, self.link_map, self.names)

    def test_conflicting_descriptor_map_rejected_unrelated_comdat_allowed(self):
        conflicting = self.text + " 0002:00000000       synthetic_descriptor_0 0000000180007777     other.obj\n"
        with self.assertRaisesRegex(ValueError, "conflicting"):
            audit.parse_map_symbols(conflicting, self.names)
        other = self.text + (
            " 0001:00000000       ??helper 0000000180001111     a.obj\n"
            " 0001:00000000       ??helper 0000000180002222     b.obj\n"
        )
        self.assertIn(audit.TABLE_SYMBOL, audit.parse_map_symbols(other, self.names))

    def test_compiled_66_contract_and_old_67_baseline(self):
        corrected = audit.audit_manager_instructions(sample_disassembly(True), True)
        original = audit.audit_manager_instructions(sample_disassembly(False), False)
        self.assertEqual(corrected["stored_module_count"], 66)
        self.assertEqual(corrected["pointer_copy_lengths"], [136, 392])
        self.assertEqual(corrected["queue_limit"], 66)
        self.assertEqual(original["pointer_copy_lengths"], [536])
        self.assertEqual(original["queue_limit"], 67)

    def test_reject_old_copy_or_queue_bound_in_corrected_code(self):
        baseline = sample_disassembly(True)
        with self.assertRaises(ValueError):
            audit.audit_manager_instructions(baseline.replace("movl $0x188", "movl $0x218"), True)
        with self.assertRaises(ValueError):
            audit.audit_manager_instructions(baseline.replace("cmpq $0x41", "cmpq $0x42"), True)
        with self.assertRaises(ValueError):
            audit.audit_manager_instructions(baseline.replace("leaq 0x90", "leaq 0x88"), True)

    def test_reject_wrong_static_insertion_index(self):
        wrong = list(self.names)
        wrong[17] = "some_other_descriptor"
        with self.assertRaisesRegex(ValueError, "Chest Stealer"):
            audit.audit_linked_pointer_table(self.binary, self.link_map, wrong)

if __name__ == "__main__":
    unittest.main()
