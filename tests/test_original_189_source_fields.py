"""R357 synthetic ClassFile negative controls (no Minecraft binaries)."""
import struct
import unittest

from original_189_source_fields_audit import audit_entries, class_fields


def u2(n):
    return struct.pack(">H", n)


def sample_class(owner, fields):
    pool = []
    def utf8(s):
        encoded = s.encode("ascii")
        pool.append(bytes([1]) + u2(len(encoded)) + encoded)
        return len(pool)
    def owner_class(name):
        idx = utf8(name)
        pool.append(bytes([7]) + u2(idx))
        return len(pool)
    this = owner_class(owner)
    parent = owner_class("java/lang/Object")
    field_indices = [(utf8(name), utf8(desc)) for name, desc in fields]
    # JVM ClassFile v52, no methods or class attributes.
    out = bytearray(struct.pack(">IHHH", 0xCAFEBABE, 0, 52, len(pool) + 1))
    out += b"".join(pool)
    out += u2(0x0021) + u2(this) + u2(parent)
    out += u2(0) + u2(len(field_indices))
    for name_idx, desc_idx in field_indices:
        out += u2(0x0002) + u2(name_idx) + u2(desc_idx) + u2(0)
    out += u2(0) + u2(0)
    return bytes(out)


class SourceFieldAuditTest(unittest.TestCase):
    def setUp(self):
        self.items = {
            "pr.class": sample_class("pr", [("bl", "I")]),
            "bex.class": sample_class(
                "bex", [("c", "D"), ("d", "D"), ("e", "D")]
            ),
        }

    def test_exact_obfuscated_owners_and_types(self):
        audit_entries(self.items.__getitem__)

    def test_wrong_primitive_type_rejected(self):
        self.items["bex.class"] = sample_class(
            "bex", [("c", "F"), ("d", "D"), ("e", "D")]
        )
        with self.assertRaises(ValueError):
            audit_entries(self.items.__getitem__)

    def test_wrong_class_owner_rejected(self):
        self.items["pr.class"] = sample_class("sv", [("bl", "I")])
        with self.assertRaises(ValueError):
            audit_entries(self.items.__getitem__)

    def test_missing_field_rejected(self):
        self.items["pr.class"] = sample_class("pr", [("other", "I")])
        with self.assertRaises(ValueError):
            audit_entries(self.items.__getitem__)

    def test_duplicate_field_rejected(self):
        with self.assertRaises(ValueError):
            class_fields(sample_class("pr", [("bl", "I"), ("bl", "I")]))

    def test_truncated_or_nonclass_refused(self):
        for raw in [b"", b"notaclass", self.items["pr.class"][:-5]]:
            with self.subTest(length=len(raw)), self.assertRaises(ValueError):
                if raw.startswith(b"\\xca\\xfe\\xba\\xbe") and len(raw) > 10:
                    class_fields(raw)
                else:
                    class_fields(raw)


if __name__ == "__main__":
    unittest.main()
