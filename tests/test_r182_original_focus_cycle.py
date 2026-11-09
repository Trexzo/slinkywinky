#!/usr/bin/env python3
"""R182: strict source-pinned focus-cycle receipt negative controls.

No original game process, physical cursor, JNI or OS input is invoked here.
"""
from __future__ import annotations
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from windows_vanilla189_isolated_manual import verify_r182_original_focus_cycle

def receipt(fg=0,num=0,exact=0,loss=0,regain=0,post=0,bind=0):
    return (
        "R176_ORIGINAL_WINDOW_PASSIVE_HOVER_RECEIPT_PASS "
        f"frames=64 samples=63 foreground={fg} "
        f"numeric={num} bind={bind} native_click_delivery=disabled\n"
        "R178_ORIGINAL_SOURCE_SLIDER_HOVER_WINDOW_PASS "
        f"eligible_draws=60 frames=64 actions=4 foreground={fg} "
        "native_click_delivery=disabled\n"
        "R179_ORIGINAL_NATIVE_SOURCE_TARGET_PASS "
        f"frames=64 source_draws=60 exact_hits={exact} "
        "native_click_delivery=disabled\n"
        "R182_ORIGINAL_NATIVE_FOCUS_CYCLE_PASS "
        f"frames=64 losses={loss} regains={regain} post_hits={post} "
        "native_click_delivery=disabled\n"
    )

def refuse(src,strict=False):
    try:
        verify_r182_original_focus_cycle(src,strict)
    except RuntimeError:
        return
    raise AssertionError("R182 forged/malformed or contradictory focus proof accepted")

def main():
    # Historical original R179 (all background): valid passive evidence,
    # explicitly NOT a recovered focus cycle.
    assert verify_r182_original_focus_cycle(receipt())==(0,0,0)
    refuse(receipt(),True)
    # Native sample order is separately bound to its real WGL owner.
    recovered=receipt(fg=6,num=4,exact=3,loss=1,regain=1,post=2)
    assert verify_r182_original_focus_cycle(recovered)==(1,1,2)
    assert verify_r182_original_focus_cycle(recovered,True)==(1,1,2)
    assert verify_r182_original_focus_cycle(
        receipt(fg=9,num=7,exact=5,loss=2,regain=2,post=3),True)==(2,2,3)
    # A loss with no reacquisition is an observation, never recovery.
    assert verify_r182_original_focus_cycle(
        receipt(fg=4,num=2,exact=2,loss=1,regain=0,post=0))==(1,0,0)
    refuse(receipt(fg=4,num=2,exact=2,loss=1,regain=0,post=0),True)
    refuse(receipt(fg=4,num=2,exact=2,loss=0,regain=1,post=1))
    refuse(receipt(fg=4,num=2,exact=2,loss=1,regain=1,post=3))
    refuse(receipt(fg=4,num=2,exact=2,loss=1,regain=1,post=0),True)
    refuse(receipt(fg=0,num=0,exact=0,loss=1,regain=0,post=0))
    refuse(receipt(fg=2,num=2,exact=2,loss=3,regain=0,post=0))
    refuse(receipt(fg=63,num=2,exact=2,loss=33,regain=32,post=1))
    refuse(receipt(fg=5,num=2,exact=1,loss=1,regain=1,post=2))
    refuse(receipt(fg=5,num=2,exact=3,loss=1,regain=1,post=2))
    refuse(receipt(fg=1,num=2,exact=1,loss=1,regain=1,post=1))
    refuse(receipt(fg=5,num=2,exact=2,loss=1,regain=1,post=2,bind=4))
    refuse(receipt(fg=5,num=2,exact=2,loss=1,regain=1,post=-1))
    refuse(receipt(fg=5,num=2,exact=2,loss=-1,regain=1,post=1))
    refuse(receipt(fg=5,num=2,exact=2,loss=1,regain=-1,post=1))
    refuse(receipt(fg=5,num=2,exact=2,loss=64,regain=1,post=1))
    refuse(receipt(fg=5,num=2,exact=2,loss=1,regain=1,post=1).replace(
        "frames=64 losses=", "frames=63 losses="))
    refuse(receipt(fg=5,num=2,exact=2,loss=1,regain=1,post=1).replace(
        "native_click_delivery=disabled", "native_click_delivery=enabled"))
    refuse(recovered+recovered)
    refuse("prefix="+recovered)
    refuse(recovered.replace(
        "R182_ORIGINAL_NATIVE_FOCUS_CYCLE_PASS",
        "R182_UNRECOGNIZED_NATIVE_FOCUS_CYCLE_PASS"))
    refuse(recovered.replace(
        "R179_ORIGINAL_NATIVE_SOURCE_TARGET_PASS",
        "R179_SOURCE_MISSING"))
    refuse(recovered,1)
    refuse(recovered,"true")
    print("R182_NATIVE_FOCUS_CHRONOLOGY_MARKER_NEGATIVES_PASS")
    print("R182_NO_HARDWARE_FOCUS_CYCLE_PROOF_FROM_SYNTHETIC_RECEIPTS")
    print("R182_NO_CLICK_OR_INPUT_CAPTURE_PERMISSION")

if __name__=="__main__":
    main()
