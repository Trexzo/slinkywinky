import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("r316", ROOT/"tools/audit_r316_controller_evidence.py")
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


class EvidenceTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)
        self.write(mod.HEADER_DIR+"/sample_controller.hpp", "#pragma once\n")
        self.write("reference_compat/tests/test_sample_controller.cpp", "int main(){}\n")
        self.write("reference_compat/CMakeLists.txt", "")
        self.write("CMakeLists.txt", "add_executable(sample reference_compat/tests/test_sample_controller.cpp)\nadd_test(NAME sample COMMAND sample)\n")
        self.write(mod.LEDGER, json.dumps({"schema":1,"controllers":{"sample":["reference_compat/tests/test_sample_controller.cpp"]}}))

    def write(self,path,content):
        p=self.root/path
        p.parent.mkdir(parents=True,exist_ok=True)
        p.write_text(content,encoding="utf-8")

    def test_registered(self):
        result=mod.audit(self.root)
        self.assertEqual(result["controller_headers"],1)
        self.assertEqual(result["associated_registered_test_sources"],1)
        self.assertFalse(result["assertion_coverage_verified"])

    def test_not_registered(self):
        self.write("CMakeLists.txt","add_executable(sample reference_compat/tests/test_sample_controller.cpp)\n")
        with self.assertRaises(ValueError):
            mod.audit(self.root)

    def test_deleted_source(self):
        (self.root/"reference_compat/tests/test_sample_controller.cpp").unlink()
        with self.assertRaises(ValueError):
            mod.audit(self.root)

    def test_new_header_requires_evidence(self):
        self.write(mod.HEADER_DIR+"/new_controller.hpp","#pragma once\n")
        with self.assertRaises(ValueError):
            mod.audit(self.root)

    def test_subproject(self):
        self.write("CMakeLists.txt","")
        self.write("reference_compat/CMakeLists.txt","add_executable(sample tests/test_sample_controller.cpp)\nadd_test(NAME sample COMMAND sample)\n")
        self.assertEqual(mod.audit(self.root)["controller_headers"],1)

    def test_reject_traversal(self):
        self.write(mod.LEDGER,json.dumps({"schema":1,"controllers":{"sample":["../sample.cpp"]}}))
        with self.assertRaises(ValueError):
            mod.audit(self.root)
