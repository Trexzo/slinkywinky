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
    re.compile(r"\bR177RetainedTooltipPixelsSmoke\b"),
    re.compile(r"\bR179ExactSourceTargetJniSmoke\b"),
    # R182 synthetic focus transitions must remain confined to the
    # disposable JNI test library, never the real original-game agent.
    re.compile(r"\bR182FocusCycleJniSmoke\b"),
    # R183 synthetic OS-focus status JNI must remain in disposable tests.
    re.compile(r"\bR183ExactWindowFocusJniSmoke\b"),
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

    def test_r191_offline_keyboard_fixture_never_enters_original_agent(self):
        # R191 may call the real first-party WGL keyboard router only from
        # native fixture tests. No JNI/agent bridge or Java Minecraft
        # instrumenter may invoke the synthetic handoff methods.
        forbidden = (
            "fixture_begin_keyboard(",
            "fixture_keyboard_event(",
            "fixture_end_keyboard(",
            "fixture_pointer_event(",
            "fixture_painted_frame(",
            "fixture_drain_release(",
            "fixture_finish_restore(",
            "MenuInputHandoffLedger",
        )
        real_jni = (ROOT / "src" / "jni_bridge.cpp").read_text(
            encoding="utf-8"
        )
        self.assertTrue(all(name not in real_jni for name in forbidden))
        agent_dir = ROOT / "loader" / "java"
        for path in agent_dir.rglob("*.java"):
            source = path.read_text(encoding="utf-8")
            self.assertTrue(
                all(name not in source for name in forbidden),
                f"R191 fixture-only handoff entered original agent: {path}",
            )
        retained = (ROOT / "src" / "windows_retained_menu_preview.cpp")
        model = retained.read_text(encoding="utf-8")
        self.assertIn("ledger.simulate(", model)
        self.assertIn("router.fixture_routes(navigation_)", model)
        self.assertIn("router.event(", model)
        self.assertIn("invalidate_painted_input();", model)

    def test_r197_recovered_text_and_bind_actions_remain_local_preview(self):
        # The source hit kinds already existed, but R196's retained WGL
        # dispatch default-rejected these controls. R197 must use the
        # existing MenuState preview-only admissions, not gameplay APIs.
        retained = (ROOT / "src" / "windows_retained_menu_preview.cpp").read_text(
            encoding="utf-8"
        )
        for fragment in (
            "case MenuHitKind::BindChip:",
            "menu_.begin_bind_capture(hit->catalog_index)",
            "case MenuHitKind::BindHold:",
            "menu_.toggle_bind_hold(hit->catalog_index)",
            "case MenuHitKind::RecoveredTextField:",
            "menu_.focus_recovered_text_preview(",
            "navigation_.focus_visible_row(hit->catalog_index)",
        ):
            self.assertIn(fragment, retained)
        fixture = (ROOT / "tests" / "test_windows_retained_pointer.cpp").read_text(
            encoding="utf-8"
        )
        self.assertIn("verify_r197_native_focus_receipts(context,before)", fixture)
        self.assertIn("focused->focused_text_widget==textIndex", fixture)
        self.assertIn("waiting->bind_capture_index==0u", fixture)
        self.assertIn("fixture_pointer_event(", fixture)

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
