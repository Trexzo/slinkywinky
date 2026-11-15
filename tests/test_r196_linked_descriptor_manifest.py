"""R196 synthetic PE and sanitized private-fixture regression tests.

These tests manufacture all PE bytes in memory; no third-party binary or
decrypted commercial program is bundled with the GitHub repository.
"""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "extract_r196_linked_descriptor_manifest.py"
sys.path.insert(0, str(SCRIPT.parent))
spec = importlib.util.spec_from_file_location("r196_linked_metadata", SCRIPT)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def invented_descriptor(cat=2, enabled=0, callback=0x180001100, priority=-10000):
    base = 0x180000000
    blob = bytearray(0x1000)
    blob[:2] = b"MZ"
    struct.pack_into("<I", blob, 0x3C, 0x80)
    blob[0x80:0x84] = b"PE\x00\x00"
    struct.pack_into("<HH", blob, 0x84, 0x8664, 1)
    struct.pack_into("<H", blob, 0x94, 0xF0)
    struct.pack_into("<H", blob, 0x98, 0x20B)
    struct.pack_into("<Q", blob, 0x98 + 24, base)
    s = 0x98 + 0xF0
    blob[s:s+8] = b".data\x00\x00\x00"
    struct.pack_into("<IIII", blob, s + 8, 0x500, 0x1000, 0x500, 0x200)
    struct.pack_into("<I", blob, 0x200 + 0x10, cat)
    blob[0x200 + 0x14] = enabled
    struct.pack_into("<Q", blob, 0x200 + 0x48, callback)
    struct.pack_into("<i", blob, 0x200 + 0x50, priority)
    struct.pack_into("<Q", blob, 0x200 + 0x48 + 25 * 16, base + 0x1200)
    struct.pack_into("<i", blob, 0x200 + 0x50 + 25 * 16, 10000)
    return blob, base


def invented_manifest():
    entries = []
    for compact in range(66):
        source = compact if compact < 17 else compact + 1
        count = 3 if compact < 39 else 2
        entries.append({
            "source_index": source,
            "category": compact % 5,
            "default_enabled": False,
            "handlers": [{"event": n, "priority": 100 - n} for n in range(count)],
        })
    return {
        "active_registration_count": 66,
        "callback_subscriptions": 171,
        "entries": entries,
    }


class LinkedMetadataTests(unittest.TestCase):
    def test_extract_real_field_offsets_from_synthetic_pe(self):
        blob, base = invented_descriptor()
        data = audit.extract_descriptor_metadata(blob, base + 0x1000, base)
        self.assertEqual(data["category"], 2)
        self.assertFalse(data["default_enabled"])
        self.assertEqual(data["handlers"], [
            {"event": 0, "priority": -10000},
            {"event": 25, "priority": 10000},
        ])

    def test_reject_invalid_category(self):
        blob, base = invented_descriptor(cat=5)
        with self.assertRaisesRegex(ValueError, "category"):
            audit.extract_descriptor_metadata(blob, base + 0x1000, base)

    def test_reject_unexpected_enabled_default(self):
        blob, base = invented_descriptor(enabled=2)
        with self.assertRaisesRegex(ValueError, "enabled"):
            audit.extract_descriptor_metadata(blob, base + 0x1000, base)

    def test_reject_callback_outside_pe_image(self):
        blob, base = invented_descriptor(callback=base_address() - 1)
        with self.assertRaisesRegex(ValueError, "callback outside"):
            audit.extract_descriptor_metadata(blob, base + 0x1000, base)

    def test_reject_priority_out_of_range(self):
        blob, base = invented_descriptor(priority=100001)
        with self.assertRaisesRegex(ValueError, "priority outside"):
            audit.extract_descriptor_metadata(blob, base + 0x1000, base)

    def test_reject_wrong_pe_section(self):
        blob, base = invented_descriptor()
        section = 0x98 + 0xF0
        blob[section:section + 8] = b".rdata\x00\x00"
        with self.assertRaisesRegex(ValueError, "inside .data"):
            audit.extract_descriptor_metadata(blob, base + 0x1000, base)

    def test_fixture_carries_only_metadata(self):
        fixture = audit.fixture_lines(invented_manifest())
        lines = fixture.splitlines()
        self.assertEqual(len(lines), 67)
        self.assertEqual(lines[0], "R196-LINKED-DESCRIPTORS-V1 66 171")
        self.assertTrue(lines[18].startswith("18 "))
        self.assertEqual(len(lines[1].split()), 7)
        self.assertNotIn("0x1800", fixture)

    def test_fixture_rejects_wrong_source_index(self):
        payload = invented_manifest()
        payload["entries"][17]["source_index"] = 17
        with self.assertRaisesRegex(ValueError, "source order"):
            audit.fixture_lines(payload)

    def test_fixture_rejects_subscription_drift(self):
        payload = invented_manifest()
        payload["callback_subscriptions"] = 170
        with self.assertRaisesRegex(ValueError, "totals differ"):
            audit.fixture_lines(payload)


def base_address():
    return 0x180000000


if __name__ == "__main__":
    unittest.main()
