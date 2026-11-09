#!/usr/bin/env python3
"""R181: synthetic negative controls for original-JVM foreground source hits.

This parses marker-shaped fixtures; it does NOT exercise a physical mouse,
original Minecraft, or original runtime restoration.
"""
from __future__ import annotations
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from windows_vanilla189_isolated_manual import (
    verify_r181_repeated_foreground_source_hits,
)

def receipt(foreground: int, numeric: int, exact: int, bind: int = 0) -> str:
    return (
        "R176_ORIGINAL_WINDOW_PASSIVE_HOVER_RECEIPT_PASS "
        f"frames=64 samples=63 foreground={foreground} "
        f"numeric={numeric} bind={bind} native_click_delivery=disabled\n"
        "R178_ORIGINAL_SOURCE_SLIDER_HOVER_WINDOW_PASS "
        f"eligible_draws=60 frames=64 actions=4 foreground={foreground} "
        "native_click_delivery=disabled\n"
        "R179_ORIGINAL_NATIVE_SOURCE_TARGET_PASS "
        f"frames=64 source_draws=60 exact_hits={exact} "
        "native_click_delivery=disabled\n"
    )

def denied(raw: str, min_hits: int = 2) -> None:
    try:
        verify_r181_repeated_foreground_source_hits(raw, min_hits)
    except RuntimeError:
        return
    raise AssertionError("R181 incorrectly accepted missing, inconsistent, or insufficient foreground source evidence")

def main() -> None:
    assert verify_r181_repeated_foreground_source_hits(
        receipt(2, 2, 2)) == (2, 2, 2)
    assert verify_r181_repeated_foreground_source_hits(
        receipt(6, 4, 3)) == (6, 4, 3)
    assert verify_r181_repeated_foreground_source_hits(
        receipt(6, 4, 3), 3) == (6, 4, 3)
    assert verify_r181_repeated_foreground_source_hits(
        receipt(63, 60, 60), 60) == (63, 60, 60)

    denied(receipt(0, 0, 0))     # R179's historical background-only run
    denied(receipt(1, 1, 1))     # one transient sample is insufficient
    denied(receipt(4, 4, 1))
    denied(receipt(4, 1, 1))
    denied(receipt(4, 2, 0))
    denied(receipt(3, 3, 2), 3)
    denied(receipt(3, 2, 2), 3)
    denied(receipt(3, 3, 2), 60)
    denied(receipt(2, 1, 2))     # cross-counter contradiction
    denied(receipt(1, 2, 2))     # numeric cannot exceed foreground
    denied(receipt(3, 2, 3))     # source cannot exceed numeric
    denied(receipt(3, 2, 2, 2))  # two categories cannot exceed foreground
    denied(receipt(63, 61, 61))  # source draw bound
    denied(receipt(64, 2, 2))    # more foreground frames than samples
    denied(receipt(3, 3, -1))
    denied(receipt(3, 3, 2) + receipt(3, 3, 2))
    denied("prefix=" + receipt(3, 3, 2))
    denied(receipt(3, 3, 2).replace(
        "R178_ORIGINAL_SOURCE_SLIDER_HOVER_WINDOW_PASS",
        "R178_UNRECOGNIZED_SOURCE_SLIDER_HOVER_WINDOW_PASS"))
    denied(receipt(3, 3, 2).replace(
        "eligible_draws=60", "eligible_draws=59"))
    denied(receipt(3, 3, 2).replace(
        "native_click_delivery=disabled", "native_click_delivery=enabled"))
    denied(receipt(3, 3, 2), 1)
    denied(receipt(3, 3, 2), 0)
    denied(receipt(3, 3, 2), 61)
    denied(receipt(3, 3, 2), True)
    denied(receipt(3, 3, 2), 2.0)
    print("R181_REPEATED_ORIGINAL_FOREGROUND_SOURCE_HIT_NEGATIVES_PASS")
    print("R181_SYNTHETIC_MARKERS_NOT_REAL_HARDWARE_MOUSE_EVIDENCE")
    print("R181_NATIVE_CLICK_ROUTING_REMAINS_DISABLED")

if __name__ == "__main__":
    main()
