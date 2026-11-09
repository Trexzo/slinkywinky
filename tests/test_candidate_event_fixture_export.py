#!/usr/bin/env python3
"""Synthetic-only regression tests: never include an extracted commercial DLL."""
import importlib.util
import struct
import tempfile
import unittest
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "candidate_manifest_export", ROOT / "tools/export_candidate_event_manifest.py")
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)


def synthetic_catalog():
    data = bytearray(136712)
    rdata = bytearray(1727316)
    categories = [0] * 17 + [1] * 10 + [2] * 15 + [3] * 7 + [4] * 17
    for i, category in enumerate(categories):
        offset = 0x100 + i * 0x300
        name_offset = 0x20000 + i * 32
        name = ("Chams" if i == 64 else "No Hurt Cam" if i == 65 else
                f"Synthetic_{i}").encode("utf-8")
        rdata[name_offset:name_offset + len(name) + 1] = name + b"\0"
        struct.pack_into("<Q", data, offset, fixture.BASE + fixture.RDATA_RVA + name_offset)
        struct.pack_into("<I", data, offset + 0x10, category)
        struct.pack_into("<Q", rdata, fixture.DESCRIPTOR_TABLE_RVA - fixture.RDATA_RVA + i * 8,
                         fixture.BASE + fixture.DATA_RVA + offset)
        # 43 x 3 and 21 x 2 = 171 callbacks; two modules are engine-only.
        count = 3 if i < 43 else 2 if i < 64 else 0
        for event in range(count):
            struct.pack_into("<Q", data, offset + 0x48 + event * 16,
                             fixture.BASE + fixture.TEXT_RVA + 100 + i * 16 + event)
            struct.pack_into("<i", data, offset + 0x50 + event * 16, -100 - i)
    return data, rdata


class TestCandidateManifestExporter(unittest.TestCase):
    def test_synthetic_complete_inventory(self):
        data, rdata = synthetic_catalog()
        rows = fixture.read_catalog(data, rdata)
        self.assertEqual(len(rows), 66)
        self.assertEqual(sum(len(row["subscriptions"]) for row in rows), 171)
        self.assertEqual(rows[64]["name"], "Chams")
        self.assertEqual(rows[65]["name"], "No Hurt Cam")
        self.assertEqual(rows[0]["subscriptions"][0], (0, -100))

    def test_verified_module_event_bindings(self):
        data, rdata = synthetic_catalog()
        rows = fixture.read_catalog(data, rdata)
        rows[31]["name"] = "Instant Stop"
        rows[31]["category"] = 1
        rows[31]["subscriptions"] = [(16, 200)]
        rows[35]["name"] = "Item Use Fix"
        rows[35]["category"] = 4
        rows[35]["subscriptions"] = [(14, 0), (15, 0)]
        rows[42]["name"] = "No Hit Delay"
        rows[42]["category"] = 4
        rows[42]["subscriptions"] = [(14, 0)]
        rows[44]["name"] = "No Item Release"
        rows[44]["category"] = 4
        rows[44]["subscriptions"] = [(3, 0)]
        rows[45]["name"] = "No Jump Delay"
        rows[45]["category"] = 1
        rows[45]["subscriptions"] = [(17, 100)]
        rows[46]["name"] = "No Slow"
        rows[46]["category"] = 1
        rows[46]["subscriptions"] = [(21, 200)]
        rows[48]["name"] = "No Use Delay"
        rows[48]["category"] = 4
        rows[48]["subscriptions"] = [(18, 0)]
        rows[49]["name"] = "Null Move"
        rows[49]["category"] = 1
        rows[49]["subscriptions"] = [(16, 100)]
        rows[51]["name"] = "Ping Fix"
        rows[51]["category"] = 4
        rows[51]["subscriptions"] = [(2, -1000), (3, -1000)]
        rows[54]["name"] = "Reach"
        rows[54]["category"] = 0
        rows[54]["subscriptions"] = [(8, 0)]
        fixture.verify_known_original_bindings(rows)
        rows[45]["subscriptions"] = [(16, 100)]
        with self.assertRaisesRegex(ValueError, "binding drift"):
            fixture.verify_known_original_bindings(rows)
        rows[45]["subscriptions"] = [(17, 100)]
        rows[54]["name"] = "Different"
        with self.assertRaisesRegex(ValueError, "binding drift"):
            fixture.verify_known_original_bindings(rows)
        rows[54]["name"] = "Reach"
        rows[49]["subscriptions"] = [(17, 100)]
        with self.assertRaisesRegex(ValueError, "binding drift"):
            fixture.verify_known_original_bindings(rows)
        rows[49]["subscriptions"] = [(16, 100)]
        rows[31]["subscriptions"] = [(16, 100)]
        with self.assertRaisesRegex(ValueError, "binding drift"):
            fixture.verify_known_original_bindings(rows)
        rows[31]["subscriptions"] = [(16, 200)]
        rows[46]["subscriptions"] = [(21, 199)]
        with self.assertRaisesRegex(ValueError, "binding drift"):
            fixture.verify_known_original_bindings(rows)
        rows[46]["subscriptions"] = [(21, 200)]
        rows[42]["subscriptions"] = [(14, 1)]
        with self.assertRaisesRegex(ValueError, "binding drift"):
            fixture.verify_known_original_bindings(rows)
        rows[42]["subscriptions"] = [(14, 0)]
        rows[48]["subscriptions"] = [(18, 1)]
        with self.assertRaisesRegex(ValueError, "binding drift"):
            fixture.verify_known_original_bindings(rows)
        rows[48]["subscriptions"] = [(18, 0)]
        rows[35]["subscriptions"] = [(14, 0), (15, 1)]
        with self.assertRaisesRegex(ValueError, "binding drift"):
            fixture.verify_known_original_bindings(rows)
        rows[35]["subscriptions"] = [(14, 0), (15, 0)]
        rows[51]["subscriptions"] = [(2, -999), (3, -1000)]
        with self.assertRaisesRegex(ValueError, "binding drift"):
            fixture.verify_known_original_bindings(rows)
        rows[51]["subscriptions"] = [(2, -1000), (3, -1000)]
        rows[44]["subscriptions"] = [(3, -1000)]
        with self.assertRaisesRegex(ValueError, "binding drift"):
            fixture.verify_known_original_bindings(rows)

    def test_duplicate_pointer_rejected(self):
        data, rdata = synthetic_catalog()
        offset = fixture.DESCRIPTOR_TABLE_RVA - fixture.RDATA_RVA
        rdata[offset + 8:offset + 16] = rdata[offset:offset + 8]
        with self.assertRaisesRegex(ValueError, "Repeated"):
            fixture.read_catalog(data, rdata)

    def test_out_of_range_callback_rejected(self):
        data, rdata = synthetic_catalog()
        struct.pack_into("<Q", data, 0x100 + 0x48, fixture.BASE + 0x70000000)
        with self.assertRaisesRegex(ValueError, "outside"):
            fixture.read_catalog(data, rdata)

    def test_bad_category_rejected(self):
        data, rdata = synthetic_catalog()
        struct.pack_into("<I", data, 0x100 + 0x10, 9)
        with self.assertRaisesRegex(ValueError, "Invalid category"):
            fixture.read_catalog(data, rdata)

    def test_private_hash_gate_rejects_synthetic_zip(self):
        data, rdata = synthetic_catalog()
        with tempfile.TemporaryDirectory() as directory:
            archive = Path(directory) / "not-original.zip"
            with zipfile.ZipFile(archive, "w") as out:
                out.writestr("synthetic/slinky2.decrypted/data.bin", data)
                out.writestr("synthetic/slinky2.decrypted/rdata.bin", rdata)
            with self.assertRaisesRegex(ValueError, "differs from pinned candidate"):
                fixture.export(archive, Path(directory) / "output.tsv")
            self.assertFalse((Path(directory) / "output.tsv").exists())


if __name__ == "__main__":
    unittest.main()
