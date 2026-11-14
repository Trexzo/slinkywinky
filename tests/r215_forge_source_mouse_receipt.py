#!/usr/bin/env python3
"""R215 source-pixel paired Minecraft-owned GUI mouse acceptance receipt.

This checks live-client receipts plus negative controls; no hardware input
is claimed. The auto-test itself calls real GuiScreen mouse handlers with
scripted coordinates derived from the actual GL11 painted source toggle.
"""
import argparse
import re
from pathlib import Path

from r214_forge_gl11_native_bridge_receipt import (
    validate as validate_r214,
    VERSION, INITIAL, INSTALLED, BRIDGE, ACK,
    DISABLE, REENABLE, CLOSED, OWNER_CLOSE, RESTORE, SHUTDOWN, EXIT,
)

NEGATIVE="R215_SOURCE_MOUSE_NEGATIVE_EDGES_REFUSED_PASS"
SOURCE="R215_EXACT_SOURCE_PAIRED_MOUSE_ACK_PASS id="
MOUSE=re.compile(
    r"R215_EXACT_SOURCE_PAIRED_MOUSE_ACK_PASS id=(\d+) "
    r"requested=(true|false) active=(true|false) "
    r"activeRows=(\d+) frames=(\d+)"
)


def validate(log: str) -> None:
    validate_r214(log)
    if log.count(NEGATIVE)!=1:
        raise ValueError("Missing/repeated orphan, off-target, mismatched-up proof")
    pointers=MOUSE.findall(log)
    if len(pointers)!=3:
        raise ValueError("Expected three actual-source paired GUI mouse receipts")
    last_frame=0
    positions=[log.index(INITIAL),log.index(INSTALLED),log.index(NEGATIVE)]
    for i, data in enumerate(pointers, 1):
        expected="true" if i%2 else "false"
        if data[:4]!=(str(i),expected,"false","0"):
            raise ValueError("Invalid GL11 mouse source acknowledgement")
        frame=int(data[4])
        if frame<=last_frame:
            raise ValueError("Source-receipt paint frames did not advance")
        last_frame=frame
        marker=SOURCE+str(i)+" requested="
        if log.count(marker)!=1:
            raise ValueError("Duplicated or absent source mouse ack")
        positions.append(log.index(marker))
        bridge=BRIDGE+str(i)+" requested="
        ack=ACK+str(i)+" requested="
        positions += [log.index(bridge),log.index(ack)]
        if i==2: positions.append(log.index(DISABLE))
        if i==3: positions.append(log.index(REENABLE))
    positions += [log.index(CLOSED), log.index(OWNER_CLOSE),
                  log.index(RESTORE), log.index(SHUTDOWN), log.index(EXIT)]
    if positions!=sorted(positions) or len(positions)!=len(set(positions)):
        raise ValueError("Source mouse/JNI/ArrayList ordering is not causal")
    if "R215_MANUAL_GL11_SOURCE_MOUSE_READY" in log:
        raise ValueError("Auto/manual mode combined")


def selftest() -> None:
    lines=[VERSION,"username: R212GuiOffline","mod: slinkyguiinputproof",
           INITIAL, INSTALLED, NEGATIVE]
    for i, requested, frames in ((1,"true",7),(2,"false",38),(3,"true",73)):
        lines += [SOURCE+f"{i} requested={requested} active=false activeRows=0 frames={frames}",
                  BRIDGE+f"{i} requested={requested} active=false activeRows=0 frames={frames}",
                  ACK+f"{i} requested={requested} active=false generation=0"]
        if i==2:lines.append(DISABLE)
        if i==3:lines.append(REENABLE)
    lines += [CLOSED,OWNER_CLOSE,RESTORE,SHUTDOWN,EXIT]
    good="\n".join(lines)
    validate(good)
    mutants=[
        good.replace(NEGATIVE,"missing negative cases"),
        good+"\n"+NEGATIVE,
        good.replace(SOURCE+"1","wrongsource id=1"),
        good.replace(SOURCE+"2","wrongsource id=2"),
        good.replace(SOURCE+"3","wrongsource id=3"),
        good.replace("id=1 requested=true","id=1 requested=false"),
        good.replace("id=2 requested=false","id=2 requested=true"),
        good.replace("id=3 requested=true","id=3 requested=false"),
        good.replace("active=false","active=true"),
        good.replace("activeRows=0","activeRows=1"),
        good.replace("frames=7","frames=0"),
        good.replace("frames=38","frames=7"),
        good.replace("frames=73","frames=38"),
        good.replace(OWNER_CLOSE,"missing close"),
        good.replace(RESTORE,"missing restore"),
        good.replace(EXIT,"unsafe System.exit(0)"),
        good+"\n"+SOURCE+"3 requested=true active=false activeRows=0 frames=100",
        good.replace(NEGATIVE+"\n"+SOURCE+"1",SOURCE+"1"),
        good.replace(
            SOURCE+"2 requested=false active=false activeRows=0 frames=38\n"+
            BRIDGE+"2 requested=false active=false activeRows=0 frames=38",
            BRIDGE+"2 requested=false active=false activeRows=0 frames=38\n"+
            SOURCE+"2 requested=false active=false activeRows=0 frames=38"),
        good.replace(SOURCE+"3 requested=true",SOURCE+"4 requested=true"),
        good+"\nR215_MANUAL_GL11_SOURCE_MOUSE_READY",
    ]
    for index,mutant in enumerate(mutants):
        try: validate(mutant)
        except ValueError: continue
        raise AssertionError("R215 mutant accepted: "+str(index))
    print("R215_GL11_SOURCE_MOUSE_21_NEGATIVE_CONTROLS_PASS")


def main():
    parser=argparse.ArgumentParser()
    modes=parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--selftest",action="store_true")
    modes.add_argument("--log",type=Path)
    args=parser.parse_args()
    if args.selftest:selftest()
    else:
        validate(args.log.read_text(encoding="utf-8",errors="replace"))
        print("R215_REAL_FORGE_GL11_PAIRED_SOURCE_MOUSE_JNI_PASS")

if __name__=="__main__":
    main()
