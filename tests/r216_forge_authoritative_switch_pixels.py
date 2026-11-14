#!/usr/bin/env python3
"""R216 real GL11 switch pixel (source copy) vs PR2 native ack receipts.

The self-test is offline: no DLL, game, or user input. Real acceptance must
supply the isolated Forge 1.8.9 client log using --log.
"""
import argparse
import re
from pathlib import Path

from r215_forge_source_mouse_receipt import validate as prior, SOURCE
from r214_forge_gl11_native_bridge_receipt import (
    VERSION, INITIAL, INSTALLED, BRIDGE, ACK, DISABLE, REENABLE,
    CLOSED, OWNER_CLOSE, RESTORE, SHUTDOWN, EXIT
)

PIXEL="R216_GL11_ACKNOWLEDGED_SPRINT_SWITCH_PIXEL_PASS "
FORMAT=re.compile(
    r"R216_GL11_ACKNOWLEDGED_SPRINT_SWITCH_PIXEL_PASS "
    r"seq=(\d+) requested=(true|false) rgb=([0-9A-Fa-f]{6}) "
    r"preview=(true|false) frames=(\d+)"
)
TRACK=("404040","F35D12","404040","F35D12")
STATES=("false","true","false","true")

def validate(log):
    prior(log)
    points=FORMAT.findall(log)
    if log.count(PIXEL)!=4 or len(points)!=4:
        raise ValueError("Four exact independent GL11 pixel receipts required")
    frame=0
    observed=[]
    for i,(seq,requested,rgb,preview,frames) in enumerate(points,1):
        if int(seq)!=i or requested!=STATES[i-1] or \
           rgb.upper()!=TRACK[i-1] or preview!="false" or int(frames)<=frame:
            raise ValueError("Unacknowledged color, preview mutation, or stale frame")
        frame=int(frames)
        observed.append(log.index(PIXEL+f"seq={i} "))
    order=[log.index(INITIAL),log.index(INSTALLED),observed[0]]
    for i in range(1,4):
        source=SOURCE+str(i)+" requested="
        bridge=BRIDGE+str(i)+" requested="
        ack=ACK+str(i)+" requested="
        order.extend([log.index(source),log.index(bridge),log.index(ack)])
        order.append(observed[i])
        if i==2:order.append(log.index(DISABLE))
        if i==3:order.append(log.index(REENABLE))
    # Existing R215 receipt must be ordered; compare additional paint
    # snapshots only with native ownership and ack markers.
    if not (observed[0]<log.index(SOURCE+"1 requested=")<
            observed[1]<log.index(SOURCE+"2 requested=")<
            observed[2]<log.index(SOURCE+"3 requested=")<
            observed[3]<log.index(CLOSED)<log.index(OWNER_CLOSE)<
            log.index(RESTORE)<log.index(SHUTDOWN)<log.index(EXIT)):
        raise ValueError("Source/ack pixel sequence or lifecycle was not causal")

def fixture():
    log=[VERSION,"username=R212GuiOffline","slinkyguiinputproof",INITIAL,INSTALLED]
    log.append(PIXEL+"seq=1 requested=false rgb=404040 preview=false frames=7")
    log.append("R215_SOURCE_MOUSE_NEGATIVE_EDGES_REFUSED_PASS")
    for i,requested,draws in ((1,"true",7),(2,"false",38),(3,"true",73)):
        log.extend([
            SOURCE+f"{i} requested={requested} active=false activeRows=0 frames={draws}",
            BRIDGE+f"{i} requested={requested} active=false activeRows=0 frames={draws}",
            ACK+f"{i} requested={requested} active=false generation=0",
            PIXEL+f"seq={i+1} requested={requested} rgb="+
               TRACK[i]+f" preview=false frames={draws+24}",
        ])
        if i==2:log.append(DISABLE)
        if i==3:log.append(REENABLE)
    log.extend([CLOSED,OWNER_CLOSE,RESTORE,SHUTDOWN,EXIT])
    return "\n".join(log)

def selftest():
    good=fixture()
    validate(good)
    cases=[
        good.replace(VERSION,"Forge version unknown"),
        good.replace("R212GuiOffline","OnlineUser"),
        good.replace(INITIAL,"no native initial state"),
        good.replace(PIXEL+"seq=1","missing-first-pixel seq=1"),
        good.replace(PIXEL+"seq=2","missing-second-pixel seq=2"),
        good.replace(PIXEL+"seq=3","missing-third-pixel seq=3"),
        good.replace(PIXEL+"seq=4","missing-fourth-pixel seq=4"),
        good.replace("rgb=404040","rgb=F35D12"),
        good.replace("rgb=F35D12","rgb=404040"),
        good.replace("rgb=F35D12","rgb=FFFFFF"),
        good.replace("rgb=404040","rgb=000000"),
        good.replace("preview=false","preview=true"),
        good.replace("seq=2 requested=true","seq=2 requested=false"),
        good.replace("seq=3 requested=false","seq=3 requested=true"),
        good.replace("seq=4 requested=true","seq=4 requested=false"),
        good.replace("seq=4","seq=5"),
        good.replace("frames=31","frames=7"),
        good.replace("frames=62","frames=31"),
        good.replace("frames=97","frames=62"),
        good.replace(CLOSED,"missing close"),
        good.replace(EXIT,"unsafe exit"),
        good+"\n"+PIXEL+"seq=4 requested=true rgb=F35D12 preview=false frames=99",
        good.replace(
            PIXEL+"seq=2 requested=true rgb=F35D12 preview=false frames=31\n"+
            SOURCE+"2",
            SOURCE+"2 requested=false active=false activeRows=0 frames=38\n"+
            PIXEL+"seq=2 requested=true rgb=F35D12 preview=false frames=31\n"+
            SOURCE+"2"),
        good.replace(ACK+"1 requested=true active=false generation=0\n"+
                     PIXEL+"seq=2",
                     PIXEL+"seq=2 requested=true rgb=F35D12 preview=false frames=31\n"+
                     ACK+"1 requested=true active=false generation=0\n"+
                     PIXEL+"seq=2"),
    ]
    for i,case in enumerate(cases):
        try:validate(case)
        except ValueError:continue
        raise AssertionError("Bad GL11 receipt accepted: "+str(i))
    print("R216_GL11_AUTHORITATIVE_SWITCH_24_NEGATIVE_CONTROLS_PASS")

if __name__=="__main__":
    p=argparse.ArgumentParser()
    group=p.add_mutually_exclusive_group(required=True)
    group.add_argument("--selftest",action="store_true")
    group.add_argument("--log",type=Path)
    a=p.parse_args()
    if a.selftest:selftest()
    else:
        validate(a.log.read_text(encoding="utf-8",errors="replace"))
        print("R216_REAL_FORGE_GL11_SWITCH_PIXELS_NATIVE_ACK_PASS")
