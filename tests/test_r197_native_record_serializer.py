"""R197 pure, synthetic private binary-record serialization regression tests.

The tests never import or contain a real native DLL, native address,
proprietary bytes, or commercial decrypted source.
"""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

FILE = Path(__file__).resolve().parents[1] / "tools" / "export_r197_private_native_records.py"
sys.path.insert(0, str(FILE.parent))
spec = importlib.util.spec_from_file_location("r197_native_records", FILE)
r197 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r197)


def synthetic_records():
    return [
        (compact if compact < 17 else compact + 1,
         bytes([(compact * 3) % 251]) * r197.MODULE_SIZE)
        for compact in range(r197.RECORD_COUNT)
    ]


class NativeRecordSerializerTests(unittest.TestCase):
    def test_exact_record_layout_with_only_synthetic_bytes(self):
        fixture = r197.serialize_records(synthetic_records(), bytes(32))
        self.assertEqual(len(fixture), 48 + 66 * (4 + 0x1f8))
        magic, count, size, image_hash = r197.HEADER.unpack_from(fixture)
        self.assertEqual(magic, b"R197REC1")
        self.assertEqual(count, 66)
        self.assertEqual(size, 504)
        self.assertEqual(image_hash, bytes(32))
        self.assertEqual(struct.unpack_from("<I", fixture, 48)[0], 0)
        self.assertEqual(struct.unpack_from("<I", fixture, 48 + 17 * 508)[0], 18)
        self.assertEqual(struct.unpack_from("<I", fixture, 48 + 65 * 508)[0], 66)

    def test_reject_short_binary_record(self):
        entries = synthetic_records()
        entries[4] = (4, bytes(503))
        with self.assertRaisesRegex(ValueError, "exact 0x1f8"):
            r197.serialize_records(entries, bytes(32))

    def test_reject_oversized_binary_record(self):
        entries = synthetic_records()
        entries[22] = (23, bytes(505))
        with self.assertRaisesRegex(ValueError, "exact 0x1f8"):
            r197.serialize_records(entries, bytes(32))

    def test_reject_missing_entry(self):
        with self.assertRaisesRegex(ValueError, "66"):
            r197.serialize_records(synthetic_records()[:-1], bytes(32))

    def test_reject_wrong_header_digest_size(self):
        with self.assertRaisesRegex(ValueError, "32 digest"):
            r197.serialize_records(synthetic_records(), bytes(31))

    def test_reject_chest_stealer_index_not_removed(self):
        entries = synthetic_records()
        entries[17] = (17, bytes(504))
        with self.assertRaisesRegex(ValueError, "order/index"):
            r197.serialize_records(entries, bytes(32))

    def test_reject_reordered_last_records(self):
        entries = synthetic_records()
        entries[-2], entries[-1] = entries[-1], entries[-2]
        with self.assertRaisesRegex(ValueError, "order/index"):
            r197.serialize_records(entries, bytes(32))


if __name__ == "__main__":
    unittest.main()
