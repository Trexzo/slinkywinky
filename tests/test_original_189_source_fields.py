"""R357 synthetic ClassFile negative controls (no Minecraft binaries)."""
import struct
import unittest

from original_189_source_fields_audit import (
    audit_entries, audit_original_attack_method, audit_original_network_send, class_fields
)


def u2(n):
    return struct.pack(">H", n)


def sample_class(owner, fields, methods=()):
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
    method_indices = [(utf8(name), utf8(desc)) for name, desc in methods]
    code_name = utf8("Code") if methods else 0
    # JVM ClassFile v52, minimal executable method bodies, no native bytes.
    out = bytearray(struct.pack(">IHHH", 0xCAFEBABE, 0, 52, len(pool) + 1))
    out += b"".join(pool)
    out += u2(0x0021) + u2(this) + u2(parent)
    out += u2(0) + u2(len(field_indices))
    for name_idx, desc_idx in field_indices:
        out += u2(0x0002) + u2(name_idx) + u2(desc_idx) + u2(0)
    out += u2(len(method_indices))
    for name_idx, desc_idx in method_indices:
        # public instance method with one RETURN instruction.
        out += u2(1) + u2(name_idx) + u2(desc_idx) + u2(1)
        payload = u2(0) + u2(1) + struct.pack(">I", 1) + bytes([0xB1]) + u2(0) + u2(0)
        out += u2(code_name) + struct.pack(">I", len(payload)) + payload
    out += u2(0)
    return bytes(out)


class SourceFieldAuditTest(unittest.TestCase):
    def setUp(self):
        self.items = {
            "pk.class": sample_class("pk", [("W", "I")]),
            "pr.class": sample_class("pr", [("bl", "I")]),
            "bex.class": sample_class(
                "bex", [("c", "D"), ("d", "D"), ("e", "D")]
            ),
        }

    def test_original_network_send_mapping(self):
        self.items["ek.class"] = sample_class("ek", [], [("a", "(Lff;)V")])
        self.items["ff.class"] = sample_class("ff", [])
        sha=audit_original_network_send(self.items.__getitem__)
        self.assertEqual(len(sha),64)
        self.assertTrue(all(c in "0123456789abcdef" for c in sha))

    def test_original_network_send_wrong_packet_signature(self):
        self.items["ek.class"] = sample_class("ek", [], [("a", "(Lft;)V")])
        self.items["ff.class"] = sample_class("ff", [])
        with self.assertRaises(ValueError):
            audit_original_network_send(self.items.__getitem__)

    def test_original_network_send_wrong_class_owner(self):
        self.items["ek.class"] = sample_class("ej", [], [("a", "(Lff;)V")])
        self.items["ff.class"] = sample_class("ff", [])
        with self.assertRaises(ValueError):
            audit_original_network_send(self.items.__getitem__)

    def test_original_network_send_wrong_packet_class(self):
        self.items["ek.class"] = sample_class("ek", [], [("a", "(Lff;)V")])
        self.items["ff.class"] = sample_class("ft", [])
        with self.assertRaises(ValueError):
            audit_original_network_send(self.items.__getitem__)

    def test_original_click_mouse_mapping(self):
        self.items["ave.class"] = sample_class("ave", [], [("s", "()V"), ("aw", "()V")])
        audit_original_attack_method(self.items.__getitem__)

    def test_wrong_click_mapping_rejected(self):
        self.items["ave.class"] = sample_class("ave", [], [("s", "()V"), ("al", "()V")])
        with self.assertRaises(ValueError):
            audit_original_attack_method(self.items.__getitem__)

    def test_wrong_click_descriptor_rejected(self):
        self.items["ave.class"] = sample_class("ave", [], [("s", "()V"), ("aw", "(I)V")])
        with self.assertRaises(ValueError):
            audit_original_attack_method(self.items.__getitem__)

    def test_missing_original_tick_rejected(self):
        self.items["ave.class"] = sample_class("ave", [], [("aw", "()V")])
        with self.assertRaises(ValueError):
            audit_original_attack_method(self.items.__getitem__)

    def test_wrong_click_owner_rejected(self):
        self.items["ave.class"] = sample_class("client", [], [("s", "()V"), ("aw", "()V")])
        with self.assertRaises(ValueError):
            audit_original_attack_method(self.items.__getitem__)

    def test_exact_obfuscated_owners_and_types(self):
        audit_entries(self.items.__getitem__)

    def test_original_entity_tick_type_rejected(self):
        self.items["pk.class"] = sample_class("pk", [("W", "J")])
        with self.assertRaises(ValueError):
            audit_entries(self.items.__getitem__)

    def test_original_entity_tick_missing_rejected(self):
        self.items["pk.class"] = sample_class("pk", [("S", "I")])
        with self.assertRaises(ValueError):
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
                class_fields(raw)


if __name__ == "__main__":
    unittest.main()
