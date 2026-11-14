#!/usr/bin/env python3
"""R217 source-backed one-pass native GL11 render receipts.

No original Minecraft mouse interception. The real client verifies one
startup navigation draw, then exactly one native menu draw per GUI paint.
"""
import argparse
import re
from pathlib import Path

from r216_forge_authoritative_switch_pixels import (
    validate as validate_r216, fixture, PIXEL
)
from r215_forge_source_mouse_receipt import SOURCE

PASS="R217_SINGLE_PASS_GL11_ACKNOWLEDGED_SWITCH_PASS "
RECEIPT=re.compile(
    r"R217_SINGLE_PASS_GL11_ACKNOWLEDGED_SWITCH_PASS "
    r"seq=(\d+) calls=(\d+) nativeFrames=(\d+) extra=(\d+)"
)
PIXEL_RE=re.compile(
    r"R216_GL11_ACKNOWLEDGED_SPRINT_SWITCH_PIXEL_PASS "
    r"seq=(\d+) requested=(true|false) rgb=([0-9A-Fa-f]{6}) "
    r"preview=false frames=(\d+)"
)

def validate(log):
    validate_r216(log)
    receipts=RECEIPT.findall(log)
    pixels=PIXEL_RE.findall(log)
    if len(receipts)!=4 or log.count(PASS)!=4 or len(pixels)!=4:
        raise ValueError("Missing/repeated R217 native paint receipts")
    last_calls=0
    for i,(rec,pixel) in enumerate(zip(receipts,pixels),1):
        seq,calls,native,extra=map(int,rec)
        p_seq,requested,rgb,frame=pixel
        if seq!=i or str(seq)!=p_seq or native!=int(frame) or \
           extra!=1 or native!=calls+1 or calls<=last_calls:
            raise ValueError("R217 duplicate, missing or unsynchronized GL11 render")
        last_calls=calls
        old=PIXEL+f"seq={seq} requested="
        new=PASS+f"seq={seq} calls={calls} nativeFrames={native} extra=1"
        if log.count(new)!=1 or \
           not log.index(old)<log.index(new):
            raise ValueError("R217 source pixel causality missing")
        if i<4 and log.index(new)>log.index(SOURCE+str(i)+" requested="):
            raise ValueError("R217 pixel receipt emitted after next request")

def good_fixture():
    text=fixture()
    for pixel in PIXEL_RE.finditer(fixture()):
        seq=int(pixel.group(1))
        frame=int(pixel.group(4))
        original=pixel.group(0)
        addition=PASS+f"seq={seq} calls={frame-1} nativeFrames={frame} extra=1"
        text=text.replace(original,original+"\n"+addition,1)
    return text

def selftest():
    valid=good_fixture()
    validate(valid)
    examples=[
        valid.replace(PASS+"seq=1","missing seq=1"),
        valid.replace(PASS+"seq=2","missing seq=2"),
        valid.replace(PASS+"seq=3","missing seq=3"),
        valid.replace(PASS+"seq=4","missing seq=4"),
        valid+"\n"+PASS+"seq=4 calls=97 nativeFrames=98 extra=1",
        valid.replace("seq=2 calls=30 nativeFrames=31 extra=1",
                      "seq=2 calls=31 nativeFrames=31 extra=1"),
        valid.replace("seq=2 calls=30 nativeFrames=31 extra=1",
                      "seq=2 calls=29 nativeFrames=31 extra=1"),
        valid.replace("seq=3 calls=61 nativeFrames=62 extra=1",
                      "seq=3 calls=61 nativeFrames=61 extra=1"),
        valid.replace("seq=4 calls=96 nativeFrames=97 extra=1",
                      "seq=4 calls=97 nativeFrames=97 extra=1"),
        valid.replace("seq=1 calls=6 nativeFrames=7 extra=1",
                      "seq=1 calls=6 nativeFrames=7 extra=2"),
        valid.replace("seq=2 calls=30","seq=3 calls=30"),
        valid.replace("seq=4 calls=96","seq=3 calls=96"),
        valid.replace("seq=3 calls=61","seq=1 calls=61"),
        valid.replace("seq=2 calls=30","seq=2 calls=0"),
        valid.replace("nativeFrames=31 extra=1","nativeFrames=999 extra=1"),
        valid.replace("nativeFrames=97 extra=1","nativeFrames=1 extra=1"),
        valid.replace("seq=1 calls=6","seq=1 calls=0"),
        valid.replace("seq=2 calls=30","seq=2 calls=5"),
        valid.replace("seq=3 calls=61","seq=3 calls=6"),
        valid.replace(PASS+"seq=4","not-one-pass seq=4"),
    ]
    for i,bad in enumerate(examples):
        try:validate(bad)
        except ValueError:continue
        raise AssertionError(f"R217 negative control accepted: {i}")
    print(f"R217_SINGLE_PASS_GL11_{len(examples)}_NEGATIVE_CONTROLS_PASS")

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    modes=parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--selftest",action="store_true")
    modes.add_argument("--log",type=Path)
    args=parser.parse_args()
    if args.selftest:selftest()
    else:
        validate(args.log.read_text(encoding="utf-8",errors="replace"))
        print("R217_REAL_FORGE_GL11_SINGLE_PASS_JNI_ACK_PASS")
