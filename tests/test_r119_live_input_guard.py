"""R119: source-only input-admission regression; no game or OS hooks."""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

# Any future live input interceptor must first replace this negative guard
# with a verified full-method owner/restore proof and an accepted runtime test.
FORBIDDEN = (
    re.compile(r"OriginalKeyboardBridgeWeaver\s*\.\s*weavePinnedOriginal\s*\("),
    re.compile(r"KeyboardQueueOwner\s*\.\s*(?:arm|bind)\s*\("),
    # R160 mouse queue is dormant. A callsite needs independently verified
    # full-input suppression and original-bytecode restoration.
    re.compile(r"MouseQueueOwner\s*\.\s*(?:arm|bind|next)\s*\("),
    # R164/R165 remain test-only. Their synthetic flags are NOT real
    # original-game ownership, foreground or bytecode-restore receipts.
    re.compile(r"\bR164OfflineModalGate\b"),
    re.compile(r"\bR165ModalInputSession\b"),
    re.compile(r"\bR166RetainedPointerJniSmoke\b"),
    re.compile(r"\bR167RecoveredSliderJniSmoke\b"),
    re.compile(r"\bR168RecoveredBooleanJniSmoke\b"),
    re.compile(r"\bR169DropdownJniSmoke\b"),
    re.compile(r"\bR175RetainedHoverJniSmoke\b"),
)


class TestDormantOriginalKeyboardInput(unittest.TestCase):
    def test_no_production_caller_arms_queue_or_installs_one_call_bridge(self):
        source_paths = []
        for base, extensions in (
            (ROOT / "src", {".cpp", ".hpp"}),
            (ROOT / "include", {".hpp"}),
            (ROOT / "loader" / "java", {".java"}),
        ):
            source_paths.extend(
                p for p in base.rglob("*")
                if p.is_file() and p.suffix in extensions
            )
        self.assertGreater(len(source_paths), 15)
        violations = []
        for path in source_paths:
            source = path.read_text(encoding="utf-8")
            for forbidden in FORBIDDEN:
                if forbidden.search(source):
                    violations.append(
                        (str(path.relative_to(ROOT)), forbidden.pattern)
                    )
        self.assertEqual(
            violations, [],
            "R119 unverified live gameplay-keyboard input interception introduced",
        )

    def test_source_pin_is_only_in_test_audit_and_existing_dormant_weaver(self):
        agent_file = ROOT / "loader" / "java" / "dev" / "slinkywinky" / "instrument" / "OriginalKeyboardBridgeWeaver.java"
        text = agent_file.read_text(encoding="utf-8")
        self.assertIn(
            "430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82",
            text,
        )
        audit_file = ROOT / "tests" / "java" / "dev" / "slinkywinky" / "attach" / "OriginalInputOwnershipAudit.java"
        audit = audit_file.read_text(encoding="utf-8")
        self.assertIn("rawKeyDown==17", audit)
        self.assertIn("heldBindingPolls==13", audit)
        self.assertIn("pressBindingPolls==3", audit)


if __name__ == "__main__":
    unittest.main()
