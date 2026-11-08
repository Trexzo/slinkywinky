#!/usr/bin/env python3
"""R179: do not confuse source-target geometry receipts with actual cursor proof."""
from __future__ import annotations
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from windows_vanilla189_isolated_manual import verify_r179_original_native_target

REAL=("R179_ORIGINAL_NATIVE_SOURCE_TARGET_PASS "
      "frames=64 source_draws=60 exact_hits=0 "
      "native_click_delivery=disabled")

def refused(sample,foreground=0,numeric=0):
    try:verify_r179_original_native_target(sample,foreground,numeric)
    except RuntimeError:return
    raise AssertionError("R179 forged source target or foreground admitted")

def main():
    assert verify_r179_original_native_target(REAL+"\n",0,0)==0
    assert verify_r179_original_native_target(
        REAL.replace("exact_hits=0","exact_hits=2")+"\r\n",7,4)==2
    assert verify_r179_original_native_target(
        REAL.replace("exact_hits=0","exact_hits=60"),63,63)==60
    refused(None)
    refused("")
    refused("prefix="+REAL)
    refused(REAL+" suffix")
    refused(REAL+"\n"+REAL)
    refused(REAL.replace("frames=64","frames=63"))
    refused(REAL.replace("source_draws=60","source_draws=59"))
    refused(REAL.replace("source_draws=60","source_draws=61"))
    refused(REAL.replace("exact_hits=0","exact_hits=-1"))
    refused(REAL.replace("exact_hits=0","exact_hits=61"),63,63)
    refused(REAL.replace("exact_hits=0","exact_hits=1"),0,0)
    refused(REAL.replace("exact_hits=0","exact_hits=2"),5,1)
    refused(REAL.replace("exact_hits=0","exact_hits=9"),5,10)
    refused(REAL.replace("native_click_delivery=disabled",
                         "native_click_delivery=enabled"))
    refused("R179_ORIGINAL_NATIVE_SOURCE_TARGET_PASS\n"+REAL)
    print("R179_EXACT_NATIVE_SOURCE_TARGET_MARKER_NEGATIVES_PASS")
    print("R179_NO_FOREGROUND_TOOLTIP_CLAIM_FROM_SOURCE_GEOMETRY")
    print("R179_SOURCE_RECEIPT_NEVER_AUTHORIZES_LIVE_INPUT")


if __name__=="__main__":main()
