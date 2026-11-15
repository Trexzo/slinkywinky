"""R192: synthetic, read-only tests for the native build readiness preflight."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import types
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "probe_openslinky_native_toolchain.py"
spec = importlib.util.spec_from_file_location("native_preflight", SCRIPT)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

class ProbeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for part in audit.REQUIRED_FILES:
            target = self.root / part
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text("synthetic only", encoding="utf-8")
        self.which = lambda name: "/test/bin/" + name
        self.git = lambda *a, **k: types.SimpleNamespace(
            returncode=0, stdout=audit.PINNED_SHA + "\n"
        )

    def test_exact_pinned_complete_toolchain_is_only_ready_case(self):
        result = audit.probe(self.root, self.which, self.git)
        self.assertTrue(result["toolchain_ready"])
        self.assertTrue(result["pinned_source_verified"])
        self.assertEqual(result["missing_tools"], [])
        self.assertFalse(result["dll_compiled"])
        self.assertFalse(result["minecraft_tested"])
        self.assertFalse(result["fidelity_verified"])

    def test_wrong_commit_is_explicitly_refused(self):
        wrong = lambda *a, **k: types.SimpleNamespace(
            returncode=0, stdout="0" * 40 + "\n"
        )
        result = audit.probe(self.root, self.which, wrong)
        self.assertFalse(result["toolchain_ready"])
        self.assertFalse(result["pinned_source_verified"])

    def test_missing_tool_is_explicit_and_nonpromoting(self):
        missing = lambda name: None if name == "llvm-dlltool" else "/test/" + name
        result = audit.probe(self.root, missing, self.git)
        self.assertEqual(result["missing_tools"], ["llvm-dlltool"])
        self.assertFalse(result["toolchain_ready"])

    def test_missing_source_file_is_refused(self):
        (self.root / "src" / "data" / "iat.s").unlink()
        result = audit.probe(self.root, self.which, self.git)
        self.assertIn("src/data/iat.s", result["missing_source_files"])
        self.assertFalse(result["source_layout_verified"])
        self.assertFalse(result["pinned_source_verified"])
        self.assertFalse(result["toolchain_ready"])

    def test_git_failure_cannot_be_a_pinned_match(self):
        fail = lambda *a, **k: types.SimpleNamespace(returncode=1, stdout="")
        result = audit.probe(self.root, self.which, fail)
        self.assertIsNone(result["observed_sha"])
        self.assertFalse(result["toolchain_ready"])

    def test_timeout_cannot_be_a_pinned_match(self):
        def timeout(*args, **kwargs):
            raise subprocess.TimeoutExpired(cmd="git", timeout=10)
        result = audit.probe(self.root, self.which, timeout)
        self.assertIsNone(result["observed_sha"])
        self.assertFalse(result["toolchain_ready"])

if __name__ == "__main__":
    unittest.main()
