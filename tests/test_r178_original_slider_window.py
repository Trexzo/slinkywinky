#!/usr/bin/env python3
"""R178 exact-source original vanilla slider-window receipt negative controls."""
from __future__ import annotations
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from windows_vanilla189_isolated_manual import (
    verify_r178_original_slider_window,
)

VALID=("R178_ORIGINAL_SOURCE_SLIDER_HOVER_WINDOW_PASS "
       "eligible_draws=60 frames=64 actions=4 foreground=0 "
       "native_click_delivery=disabled")


def rejected(source: str, foreground: int = 0) -> None:
    try:
        verify_r178_original_slider_window(source,foreground)
    except RuntimeError:
        return
    raise AssertionError("R178 forged or ambiguous slider-window evidence accepted")


def main() -> None:
    assert verify_r178_original_slider_window(VALID+"\n",0)==60
    assert verify_r178_original_slider_window(
        "R176_ORIGINAL_WINDOW_PASSIVE_HOVER_RECEIPT_PASS\n"+VALID+"\r\n",0)==60
    assert verify_r178_original_slider_window(
        VALID.replace("foreground=0","foreground=12")+"\n",12)==60
    rejected("")
    rejected(None)
    rejected(VALID,1)
    rejected(VALID.replace("foreground=0","foreground=12"),11)
    rejected(VALID+"\n"+VALID)
    rejected("prefix="+VALID)
    rejected(VALID+" suffix\n")
    rejected(VALID.replace("eligible_draws=60","eligible_draws=59"))
    rejected(VALID.replace("eligible_draws=60","eligible_draws=64"))
    rejected(VALID.replace("frames=64","frames=63"))
    rejected(VALID.replace("actions=4","actions=3"))
    rejected(VALID.replace("foreground=0","foreground=64"),64)
    rejected(VALID.replace("foreground=0","foreground=-1"),-1)
    rejected(VALID.replace("native_click_delivery=disabled",
                           "native_click_delivery=enabled"))
    rejected("R178_ORIGINAL_SOURCE_SLIDER_HOVER_WINDOW_PASS\n"+VALID)
    print("R178_ORIGINAL_CLIENT_SOURCE_SLIDER_MARKER_NEGATIVES_PASS")
    print("R178_SOURCE_DWELL_DOES_NOT_ASSERT_ACTUAL_PHYSICAL_HOVER")
    print("R178_NATIVE_CLICK_ROUTING_REMAINS_DISABLED")


if __name__=="__main__":
    main()
