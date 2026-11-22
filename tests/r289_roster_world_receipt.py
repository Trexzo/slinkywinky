#!/usr/bin/env python3
"""R289 receipt checker. --selftest uses fabricated logs only."""
import argparse
import re
from pathlib import Path

MARKERS = (
    "R285_INDEPENDENT_ROSTER_TEMP_GATE_PASS",
    "R289_ROSTER_WORLD_ISOLATION_GATE_PASS",
    "R289_LOCAL_WORLD_REQUESTED",
    "R289_LOCAL_WORLD_PLAYER_READY_PASS",
    "R283_FORGE_NATIVE_ANTIBOT_READONLY_RECEIPT",
    "R289_REAL_WORLD_ANTIBOT_NATIVE_JNI_PASS",
    "R289_ROSTER_NATIVE_CLEANUP_PASS",
)
RECEIPT = re.compile(
    r"R289_REAL_WORLD_ANTIBOT_NATIVE_JNI_PASS "
    r"samples=(\d+) clientTicks=(\d+) remoteCount=(\d+) "
    r"generation=(\d+) gameWrites=0 "
    r"source=Forge_PlayerTick_END_JNI")
CLEAN = ("R289_ROSTER_NATIVE_CLEANUP_PASS "
    "native_owner_closed=1 preview_closed=1 "
    "auto_restart=0 gameWrites=0")
BAD = ("R289_PREVIEW_START_FAIL","R289_ROSTER_WORLD_FAIL",
    "R278_ROSTER_READONLY_FAIL","R283_NATIVE_ROSTER_READONLY_FAIL",
    "UnsatisfiedLinkError","Exception in thread")

def validate(log):
    if any(log.count(m)!=1 for m in MARKERS):
        raise ValueError("Missing or repeated world/JNI marker")
    positions = [log.index(m) for m in MARKERS]
    if positions!=sorted(positions):
        raise ValueError("World/JNI/cleanup causal order invalid")
    if any(x in log for x in BAD) or log.count(CLEAN)!=1:
        raise ValueError("Failure or incomplete native cleanup")
    values=RECEIPT.findall(log)
    if len(values)!=1:
        raise ValueError("Missing complete JNI receipt")
    samples,ticks,remote,generation=map(int,values[0])
    if samples!=32 or ticks<32 or ticks>10000 or remote>256 or generation<1:
        raise ValueError("Invalid real-world sample counts")
    return samples,ticks,remote,generation

def fixture():
    return "\n".join((
        "[SlinkyWinkyPreview] "+MARKERS[0],
        "[SlinkyWinkyPreview] "+MARKERS[1],
        "[SlinkyWinkyPreview] "+MARKERS[2],
        "[SlinkyWinkyPreview] "+MARKERS[3],
        "[SlinkyWinkyPreview] R283_FORGE_NATIVE_ANTIBOT_READONLY_RECEIPT "
        "samples=20 remoteCount=0 rejectedCount=0 generation=1 "
        "proposalsOnly=1 gameWrites=0 source=Forge_PlayerTick_END_JNI",
        "[SlinkyWinkyPreview] R289_REAL_WORLD_ANTIBOT_NATIVE_JNI_PASS "
        "samples=32 clientTicks=48 remoteCount=0 generation=1 "
        "gameWrites=0 source=Forge_PlayerTick_END_JNI",
        "[SlinkyWinkyPreview] "+CLEAN))

def selftest():
    good=fixture()
    validate(good)
    mutants=[good.replace(m,"MISSING",1) for m in MARKERS]
    mutants.extend((
        good.replace("clientTicks=48","clientTicks=1"),
        good.replace("samples=32","samples=31"),
        good.replace("remoteCount=0 generation=1 gameWrites=0",
                     "remoteCount=999 generation=1 gameWrites=0"),
        good.replace("gameWrites=0 source=Forge_PlayerTick_END_JNI",
                     "gameWrites=1 source=Forge_PlayerTick_END_JNI"),
        good.replace("generation=1 gameWrites=0","generation=0 gameWrites=0"),
        good.replace("native_owner_closed=1","native_owner_closed=0"),
        good.replace("auto_restart=0","auto_restart=1"),
        good+"\n"+MARKERS[5],
        good+"\nR289_ROSTER_WORLD_FAIL synthetic",
        good.replace(MARKERS[3],MARKERS[2]).replace(
            MARKERS[2],MARKERS[3],1),
    ))
    for i,mutant in enumerate(mutants):
        try:validate(mutant)
        except ValueError:continue
        raise AssertionError("Fake R289 receipt accepted: "+str(i))
    print("R289_ROSTER_RECEIPT_%d_NEGATIVE_CONTROLS_PASS" % len(mutants))

if __name__=="__main__":
    ap=argparse.ArgumentParser()
    group=ap.add_mutually_exclusive_group(required=True)
    group.add_argument("--selftest",action="store_true")
    group.add_argument("--log",type=Path)
    args=ap.parse_args()
    if args.selftest:selftest()
    else:
        result=validate(args.log.read_text(encoding="utf-8",errors="replace"))
        print("R289_REAL_FORGE_WORLD_ROSTER_RECEIPT_VALIDATED samples=%d ticks=%d remote=%d generation=%d"%result)
