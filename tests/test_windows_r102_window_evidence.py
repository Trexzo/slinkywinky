"""R102 exact-PID/birth/window pure classifier negative controls.

Runs without Windows, a game, a JNI loader or OS input.
"""
import unittest
from windows_r102_window_identity_evidence import (
    classify_snapshot, WindowAuditError, EXPECTED_TITLE
)


def original(**changes):
    item = dict(
        pid=1234, birth=987654321, title=EXPECTED_TITLE, visible=True,
        minimized=False, enabled=True, root=True, thread_id=19,
        width=854, height=480, foreground=False,
    )
    item.update(changes)
    return item


class R102WindowIdentityTests(unittest.TestCase):
    def test_unique_background_and_foreground_status_are_distinct(self):
        self.assertTrue(classify_snapshot([original(foreground=False)],
                                          1234, 987654321)["background"])
        self.assertTrue(classify_snapshot([original(foreground=True)],
                                          1234, 987654321)["foreground"])

    def test_extra_unrelated_pid_cannot_spoof_original_window(self):
        accepted = classify_snapshot(
            [original(), original(pid=555, title=EXPECTED_TITLE)],
            1234, 987654321)
        self.assertEqual(accepted["window_count"], 1)
        self.assertFalse(accepted["render_hdc_certified"])
        self.assertFalse(accepted["input_capture_certified"])

    def test_refuse_absent_ambiguous_minimized_or_false_identity(self):
        invalid = (
            [], [original(), original()],
            [original(birth=5)], [original(pid=5)],
            [original(title="Minecraft")], [original(visible=False)],
            [original(minimized=True)], [original(enabled=False)],
            [original(root=False)], [original(thread_id=0)],
            [original(width=319)], [original(height=239)],
            [original(width=8193)], [original(height=8193)],
            [original(foreground=None)],
        )
        for value in invalid[:-1]:
            with self.subTest(value=value), self.assertRaises(WindowAuditError):
                classify_snapshot(value, 1234, 987654321)
        # Ambiguous focus must be handled as uncertain, never foreground.
        with self.assertRaises(WindowAuditError):
            classify_snapshot([original(foreground=None)], 1234, 987654321)

    def test_birth_or_pid_missing_rejected(self):
        for pid, birth in [(0,99),(123,0),(-1,2),("123",4)]:
            with self.subTest(pid=pid,birth=birth),self.assertRaises(WindowAuditError):
                classify_snapshot([original()],pid,birth)


if __name__ == "__main__":
    unittest.main()
