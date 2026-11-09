#!/usr/bin/env python3
"""R183 exact original HWND OS focus receipt synthetic negative controls."""
from __future__ import annotations
import sys
import subprocess
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from windows_vanilla189_isolated_manual import verify_r183_original_window_focus

def receipt(fg=4,num=3,exact=2,compat_post=1,os_fg=4,os_bg=59,
            refused=0,loss=1,regain=1,post=1):
    return (
        "R176_ORIGINAL_WINDOW_PASSIVE_HOVER_RECEIPT_PASS "
        f"frames=64 samples=63 foreground={fg} numeric={num} bind=0 "
        "native_click_delivery=disabled\n"
        "R178_ORIGINAL_SOURCE_SLIDER_HOVER_WINDOW_PASS "
        f"eligible_draws=60 frames=64 actions=4 foreground={fg} "
        "native_click_delivery=disabled\n"
        "R179_ORIGINAL_NATIVE_SOURCE_TARGET_PASS "
        f"frames=64 source_draws=60 exact_hits={exact} "
        "native_click_delivery=disabled\n"
        "R182_ORIGINAL_NATIVE_FOCUS_CYCLE_PASS "
        f"frames=64 losses={loss} regains={regain} post_hits={compat_post} "
        "native_click_delivery=disabled\n"
        "R183_ORIGINAL_EXACT_WINDOW_OS_FOCUS_PASS "
        f"frames=64 foreground={os_fg} background={os_bg} "
        f"refused={refused} losses={loss} regains={regain} "
        f"post_hits={post} native_click_delivery=disabled\n"
    )

def refused(source,strict=False):
    try:verify_r183_original_window_focus(source,strict)
    except RuntimeError:return
    raise AssertionError("R183 falsely accepted unverified OS focus evidence")

def main():
    # R186: exercise the actual CLI entrypoint without --run; importing
    # the helper alone missed a local 'os' shadow that crashed at startup.
    help_probe=subprocess.run(
        [sys.executable,str(Path(__file__).with_name(
            "windows_vanilla189_isolated_manual.py")),"--help"],
        capture_output=True,text=True,timeout=15)
    assert help_probe.returncode==0 and (
        "--render-retained-acceptance" in help_probe.stdout),(
        "Original vanilla CLI help failed before launching Minecraft")
    baseline=receipt()
    assert verify_r183_original_window_focus(baseline,True)==(
        4,59,0,1,1,1)
    assert verify_r183_original_window_focus(baseline,False)==(
        4,59,0,1,1,1)
    bg=receipt(fg=0,num=0,exact=0,compat_post=0,
               os_fg=0,os_bg=63,refused=0,loss=0,regain=0,post=0)
    assert verify_r183_original_window_focus(bg)==(0,63,0,0,0,0)
    refused(bg,True)
    invalid=receipt(os_fg=3,os_bg=59,refused=1)
    assert verify_r183_original_window_focus(invalid)==(3,59,1,1,1,1)
    refused(invalid,True)
    refused(receipt(os_fg=4,os_bg=58,refused=0),True)
    refused(receipt(os_fg=4,os_bg=60,refused=0))
    refused(receipt(os_fg=5,os_bg=58,refused=0))
    refused(receipt(os_fg=0,os_bg=63,refused=0))
    refused(receipt(os_fg=1,os_bg=62,refused=0,loss=2,regain=1,post=1))
    refused(receipt(os_fg=3,os_bg=60,refused=0,loss=3,regain=4,post=1))
    refused(receipt(os_fg=4,os_bg=59,refused=0,loss=4,regain=1,post=1))
    refused(receipt(os_fg=4,os_bg=59,refused=0,loss=1,regain=0,post=1))
    refused(receipt(os_fg=4,os_bg=59,refused=0,loss=1,regain=1,post=2))
    refused(receipt(os_fg=4,os_bg=59,refused=0,loss=1,regain=1,post=3))
    refused(receipt(os_fg=4,os_bg=59,refused=0,loss=1,regain=1,post=-1))
    refused(receipt(os_fg=4,os_bg=59,refused=0,loss=1,regain=-1,post=1))
    refused(receipt(os_fg=4,os_bg=59,refused=0,loss=-1,regain=1,post=1))
    refused(receipt(os_fg=4,os_bg=59,refused=-1))
    refused(receipt(os_fg=4,os_bg=59,refused=64))
    refused(receipt(fg=3,num=2,exact=1,compat_post=1,
                    os_fg=3,os_bg=60,refused=0,
                    loss=1,regain=1,post=2))
    refused(baseline+baseline)
    refused("prefix="+baseline)
    refused(baseline.replace(
        "R183_ORIGINAL_EXACT_WINDOW_OS_FOCUS_PASS",
        "R183_INVALID_ORIGINAL_EXACT_WINDOW_OS_FOCUS_PASS"))
    refused(baseline.replace(
        "frames=64 foreground=", "frames=63 foreground="))
    refused(baseline.replace(
        "native_click_delivery=disabled",
        "native_click_delivery=enabled"))
    refused(baseline,1)
    refused(baseline,"true")
    print("R186_ORIGINAL_CLIENT_CLI_MODULE_SHADOW_PREVENTION_PASS")
    print("R183_EXACT_WINDOW_FOCUS_MARKER_NEGATIVES_PASS")
    print("R183_BACKGROUND_NEVER_EQUALS_REFUSED_ADMISSION_PASS")
    print("R183_SYNTHETIC_MARKERS_NOT_ORIGINAL_GAME_OR_HARDWARE_PROOF")

if __name__=="__main__":
    main()
