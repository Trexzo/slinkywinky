#!/usr/bin/env python3
"""R218 observed-in-GuiScreen focus/close cancellation live-log receipt.

Automated focus sample is SCRIPTED; never interpret this fixture as a real
OS foreground transition or genuine physical mouse acceptance.
"""
import argparse
from pathlib import Path
from r217_forge_one_pass_source_paint import (
    validate as inherited, good_fixture, PASS
)
from r215_forge_source_mouse_receipt import SOURCE, NEGATIVE
from r214_forge_gl11_native_bridge_receipt import (
    INITIAL, INSTALLED, CLOSED, OWNER_CLOSE, RESTORE, SHUTDOWN, EXIT
)

FOCUS="R218_FOCUS_LOSS_STALE_RELEASE_REFUSED_PASS"
CLOSE="R218_HELD_CLICK_ESCAPE_CLOSE_CANCELED_PASS"
PIXEL4="R217_SINGLE_PASS_GL11_ACKNOWLEDGED_SWITCH_PASS seq=4"

def validate(log):
    inherited(log)
    if log.count(FOCUS)!=1 or log.count(CLOSE)!=1:
        raise ValueError("R218 exactly one canceled focus hold and close required")
    ordered=[
        INITIAL,INSTALLED,NEGATIVE,FOCUS,SOURCE+"1 requested=",
        SOURCE+"2 requested=",SOURCE+"3 requested=",PIXEL4,CLOSE,
        CLOSED,OWNER_CLOSE,RESTORE,SHUTDOWN,EXIT
    ]
    points=[log.find(item) for item in ordered]
    if any(p<0 for p in points) or points!=sorted(points) or \
       len(points)!=len(set(points)):
        raise ValueError("R218 focus-close/source-ack chronology violated")
    # Existing R215 validates EXACTLY three accepted JNI-source toggles,
    # even while both abandoned gestures contain legitimate mouse Down.
    if log.count(SOURCE)!=3:
        raise ValueError("R218 unapproved fourth source request")

def good_fixture_r218():
    good=good_fixture()
    good=good.replace(NEGATIVE,NEGATIVE+"\n"+FOCUS,1)
    good=good.replace(CLOSED,CLOSE+"\n"+CLOSED,1)
    return good

def selftest():
    good=good_fixture_r218()
    validate(good)
    cases=[
        good.replace(FOCUS,"focus not canceled"),
        good.replace(CLOSE,"held close not canceled"),
        good+"\n"+FOCUS,
        good+"\n"+CLOSE,
        good.replace(FOCUS+"\n"+SOURCE+"1",SOURCE+"1"),
        good.replace(NEGATIVE+"\n"+FOCUS,FOCUS+"\n"+NEGATIVE),
        good.replace(INITIAL,INITIAL+"\n"+FOCUS),
        good.replace(INSTALLED,INSTALLED+"\n"+CLOSE),
        good.replace(CLOSE+"\n"+CLOSED,CLOSED+"\n"+CLOSE),
        good.replace(CLOSE+"\n"+CLOSED,CLOSED+"\n"+RESTORE+"\n"+CLOSE),
        good.replace(PIXEL4,"R217 missing final pixel"),
        good.replace(SOURCE+"1","R215 wrong source 1"),
        good.replace(SOURCE+"2","R215 wrong source 2"),
        good.replace(SOURCE+"3","R215 wrong source 3"),
        good.replace(OWNER_CLOSE,"owner did not close"),
        good.replace(RESTORE,"vanilla not restored"),
        good.replace(EXIT,"unsupported immediate exit"),
        good+"\n"+SOURCE+"4 requested=false active=false activeRows=0 frames=200",
        good.replace(FOCUS,FOCUS+"\n"+FOCUS),
        good.replace(CLOSE,CLOSE+"\n"+CLOSE)
    ]
    for i,bad in enumerate(cases):
        try: validate(bad)
        except ValueError: continue
        raise AssertionError(f"R218 invalid receipt accepted: {i}")
    print(f"R218_FOCUS_CLOSE_GESTURE_{len(cases)}_NEGATIVE_CONTROLS_PASS")

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    group=parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--selftest",action="store_true")
    group.add_argument("--log",type=Path)
    args=parser.parse_args()
    if args.selftest:selftest()
    else:
        validate(args.log.read_text(encoding="utf-8",errors="replace"))
        print("R218_REAL_FORGE_FOCUS_LOSS_AND_ESCAPE_CANCEL_PASS")
