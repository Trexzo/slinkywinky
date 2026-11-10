"""R193 synthetic fixtures: no upstream source or proprietary data checked in."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import types
import unittest
from unittest.mock import patch

SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "prepare_openslinky_registration_66.py"
spec = importlib.util.spec_from_file_location("registration_66", SCRIPT)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def synthetic_manager():
    lines = [
        'extern "C" Module *const ptr_to_aim_assist_module_descriptor[0x43];',
        "manager->count = 0x43;",
        "manager->mods = (Module **)libc_calloc(0x43, 8);",
        "libc_memcpy_alt(manager->mods, ptr_to_aim_assist_module_descriptor, 0x218);",
        "for (unsigned long long i = 0; i != 0x43; i++) {",
        "Module *mod = ptr_to_aim_assist_module_descriptor[i];",
        "for (unsigned long long i = 0; i != 0x43; i++) {",
        "Module *mod = ptr_to_aim_assist_module_descriptor[i];",
        "manager->en_list = (Module **)libc_calloc(0x43, 8);",
        "manager->dis_list = (Module **)libc_calloc(0x43, 8);",
        "if (*n >= 0x43) return false;",
    ]
    return "\n".join(lines) + "\n"


def synthetic_table():
    vals = ["descriptor_" + str(i) for i in range(67)]
    vals[17] = "chest_stealer_module_descriptor"
    return "prefix\nptr_to_aim_assist_module_descriptor:\n" + (
        "\n".join(".quad " + name for name in vals)
    ) + "\n.globl sentinel\n"


class CorrectedRegistrationTests(unittest.TestCase):
    def test_exact_compaction_and_no_table_mutation(self):
        original = synthetic_manager()
        patched = audit.patch_manager(original)
        self.assertEqual(original.count("0x43"), 8)
        self.assertIn("manager->count = 0x42;", patched)
        self.assertEqual(patched.count("manager->mods[i]"), 2)
        self.assertEqual(patched.count("i != 0x42"), 2)
        self.assertIn("ptr_to_aim_assist_module_descriptor + 18, 0x188", patched)
        self.assertIn("ptr_to_aim_assist_module_descriptor, 0x88", patched)
        self.assertIn("ptr_to_aim_assist_module_descriptor[0x43]", patched)

    def test_reject_changed_native_manager_expression(self):
        with self.assertRaises(ValueError):
            audit.patch_manager(synthetic_manager().replace("0x218", "0x210"))

    def test_reject_duplicate_unexpected_source_operation(self):
        with self.assertRaises(ValueError):
            audit.patch_manager(synthetic_manager() + "manager->count = 0x43;\n")

    def test_table_insertion_index_is_exact(self):
        entries = audit.descriptor_entries(synthetic_table())
        self.assertEqual(len(entries), 67)
        self.assertEqual(entries[17], "chest_stealer_module_descriptor")
        self.assertEqual(len(entries[:17] + entries[18:]), 66)

    def test_wrong_position_and_duplicate_table_refused(self):
        table = synthetic_table().replace(
            ".quad chest_stealer_module_descriptor",
            ".quad descriptor_replaced",
        )
        with self.assertRaises(ValueError):
            audit.descriptor_entries(table)
        with self.assertRaises(ValueError):
            audit.descriptor_entries(synthetic_table().replace(
                ".quad descriptor_1", ".quad descriptor_0", 1
            ))

    def test_source_root_preserved_and_output_produced_only_outside_it(self):
        with tempfile.TemporaryDirectory() as folder:
            base = Path(folder)
            root = base / "upstream"
            (root / audit.MANAGER_REL).parent.mkdir(parents=True)
            (root / audit.RDATA_REL).parent.mkdir(parents=True)
            raw = synthetic_manager().encode()
            (root / audit.MANAGER_REL).write_bytes(raw)
            (root / audit.RDATA_REL).write_text(synthetic_table(), encoding="utf-8")
            sha = hashlib.sha256(raw).hexdigest()
            rdata_sha = hashlib.sha256(synthetic_table().encode()).hexdigest()
            git_result = types.SimpleNamespace(stdout=audit.UPSTREAM_SHA + "\n")
            with patch.object(audit, "MANAGER_SHA256", sha), patch.object(
                audit, "RDATA_SHA256", rdata_sha
            ), patch.object(audit.subprocess, "run", return_value=git_result):
                inside = root / "modified_manager.cpp"
                with self.assertRaises(ValueError):
                    audit.make_patch(root, inside)
                self.assertFalse(inside.exists())
                outside = base / "modified_manager.cpp"
                result = audit.make_patch(root, outside)
                self.assertTrue(outside.exists())
                self.assertEqual(result["candidate_registration_count"], 66)
                self.assertFalse(result["native_compiled"])
                self.assertEqual((root / audit.MANAGER_REL).read_bytes(), raw)
                (root / audit.RDATA_REL).write_text(
                    synthetic_table() + "# synthetic drift\n", encoding="utf-8"
                )
                drift_output = base / "drift_manager.cpp"
                with self.assertRaises(ValueError):
                    audit.make_patch(root, drift_output)
                self.assertFalse(drift_output.exists())
                with self.assertRaises(FileExistsError):
                    audit.make_patch(root, outside)


if __name__ == "__main__":
    unittest.main()
