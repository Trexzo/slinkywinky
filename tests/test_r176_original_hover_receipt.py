#!/usr/bin/env python3
"""R176: strict *log-only* original-client hover marker negative controls.

Synthetic receipts only, not original Minecraft runtime or a live input hook.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from windows_vanilla189_isolated_manual import verify_r176_original_hover_marker

MARKER = (
    "R176_ORIGINAL_WINDOW_PASSIVE_HOVER_RECEIPT_PASS "
    "frames=64 samples=63 foreground=7 numeric=3 bind=2 "
    "native_click_delivery=disabled"
)


def rejected(text: str) -> None:
    try:
        verify_r176_original_hover_marker(text)
    except RuntimeError:
        return
    raise AssertionError("R176 forged or malformed hover receipt was accepted")


def main() -> None:
    assert verify_r176_original_hover_marker(MARKER + "\n") == (7, 3, 2)
    assert verify_r176_original_hover_marker(
        "R99_WGL_CONTEXT_STABLE_PASS\n" + MARKER + "\r\n"
    ) == (7, 3, 2)
    assert verify_r176_original_hover_marker(
        MARKER.replace("foreground=7 numeric=3 bind=2",
                       "foreground=0 numeric=0 bind=0") + "\n"
    ) == (0, 0, 0)
    rejected("")
    rejected("R99_RETAINED_MENU_DRAW_PASS frames=64\n")
    rejected(" prefix=" + MARKER + "\n")
    rejected(MARKER + "SUFFIX\n")
    rejected(MARKER + "\n" + MARKER + "\n")
    rejected(MARKER + "\n" + MARKER.replace("bind=2", "bind=1") + "\n")
    rejected(MARKER.replace("frames=64", "frames=63"))
    rejected(MARKER.replace("samples=63", "samples=62"))
    rejected(MARKER.replace("samples=63", "samples=64"))
    rejected(MARKER.replace("foreground=7", "foreground=64"))
    rejected(MARKER.replace("foreground=7", "foreground=-1"))
    rejected(MARKER.replace("foreground=7", "foreground=3"))
    rejected(MARKER.replace("numeric=3", "numeric=8"))
    rejected(MARKER.replace("bind=2", "bind=8"))
    rejected(MARKER.replace("numeric=3 bind=2", "numeric=6 bind=3"))
    rejected(MARKER.replace("native_click_delivery=disabled",
                            "native_click_delivery=enabled"))
    rejected(MARKER.replace("native_click_delivery=disabled",
                            "native_click_delivery=disabled extra=1"))
    rejected("R176_ORIGINAL_WINDOW_PASSIVE_HOVER_RECEIPT_PASS\n" + MARKER)
    rejected(None)
    print("R176_ORIGINAL_GAME_HOVER_MARKER_NEGATIVE_CONTROLS_PASS")
    print("R176_NO_FOREGROUND_OR_PHYSICAL_TOOLTIP_CLAIM_FROM_ZERO_SAMPLES")
    print("R176_REPLAY_FORGERY_AND_INPUT_PERMISSION_DENIED")


if __name__ == "__main__":
    main()
