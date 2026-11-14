#!/usr/bin/env python3
"""R214 native GL11/WGL + real PR2 JNI title-screen acceptance evidence.

Selftest does not launch a game or load a DLL. Scripted GuiButton clicks
are never represented as physical mouse or reconstructed-toggle clicks.
"""
import argparse
import re
from pathlib import Path

VERSION="Forge Mod Loader version 11.15.1.2318 for Minecraft 1.8.9"
INITIAL="R214_NATIVE_GL11_SOURCE_INITIAL_OFF_PASS"
INSTALLED="R212_REAL_FORGE_GUI_SCREEN_INSTALLED"
BRIDGE="R214_REAL_GL11_MENU_TO_JNI_TO_ARRAYLIST_FEED_PASS id="
ACK="R213_REAL_JNI_NATIVE_SPRINT_ACK id="
DISABLE="R213_REAL_JNI_SPRINT_DISABLE_AND_RENDER_RECEIPT_PASS"
REENABLE="R213_REAL_JNI_SPRINT_REENABLE_AND_RENDER_RECEIPT_PASS"
CLOSED="R214_NATIVE_GL11_GUI_AND_FEED_CLOSED_PASS"
OWNER_CLOSE="R213_REAL_JNI_SPRINT_OWNER_CLOSE_PASS"
RESTORE="R212_REAL_FORGE_GUI_ESC_RESTORES_PREVIOUS_SCREEN"
SHUTDOWN="R212_ISOLATED_CLIENT_CLEAN_SHUTDOWN"
EXIT="Java has been asked to exit (code 0) by net.minecraftforge.fml.common.FMLCommonHandler.exitJava"
BRIDGE_RE=re.compile(r"R214_REAL_GL11_MENU_TO_JNI_TO_ARRAYLIST_FEED_PASS id=(\d+) requested=(true|false) active=(true|false) activeRows=(\d+) frames=(\d+)")
ACK_RE=re.compile(r"R213_REAL_JNI_NATIVE_SPRINT_ACK id=(\d+) requested=(true|false) active=(true|false) generation=(\d+)")
SEQUENCE=(INITIAL,INSTALLED,DISABLE,REENABLE,CLOSED,OWNER_CLOSE,RESTORE,SHUTDOWN,EXIT)

def validate(log):
    if VERSION not in log or "slinkyguiinputproof" not in log or "R212GuiOffline" not in log:
        raise ValueError("Wrong Forge version/test mod/identity")
    if "FMLSecurityManager$ExitTrappedException" in log:
        raise ValueError("Unsafe process exit")
    positions=[]
    for marker in SEQUENCE:
        if log.count(marker)!=1:
            raise ValueError("Missing or repeated lifecycle marker: "+marker)
        positions.append(log.index(marker))
    if positions!=sorted(positions):
        raise ValueError("Reordered lifecycle")
    bridges=BRIDGE_RE.findall(log)
    acks=ACK_RE.findall(log)
    if len(bridges)!=3 or len(acks)!=3:
        raise ValueError("Expected three exact native-owner/WGL and JNI acknowledgements")
    lastframe=0
    for i,(bridge,ack) in enumerate(zip(bridges,acks),start=1):
        req="true" if i%2 else "false"
        if bridge[:4]!=(str(i),req,"false","0") or \
           ack!=(str(i),req,"false","0"):
            raise ValueError("Wrong native or PR2 requested/active state")
        frames=int(bridge[4])
        if frames<=lastframe:
            raise ValueError("No advancing actual native GL11 painted frames")
        lastframe=frames
    ordered=[positions[0],positions[1]]
    for i in range(1,4):
        b=re.search(re.escape(BRIDGE)+str(i)+r" requested=",log)
        a=re.search(re.escape(ACK)+str(i)+r" requested=",log)
        if not b or not a:
            raise ValueError("Missing exact request ID")
        ordered += [b.start(),a.start()]
        if i==2: ordered += [log.index(DISABLE)]
        if i==3: ordered += [log.index(REENABLE)]
    ordered += [log.index(CLOSED),log.index(OWNER_CLOSE),
                log.index(RESTORE),log.index(SHUTDOWN),log.index(EXIT)]
    if ordered!=sorted(ordered) or len(ordered)!=len(set(ordered)):
        raise ValueError("GUI, source, native, close or shutdown out of order")

def selftest():
    good="\n".join([
        VERSION, "user=R212GuiOffline", "mod=slinkyguiinputproof",
        INITIAL, INSTALLED,
        BRIDGE+"1 requested=true active=false activeRows=0 frames=7",
        ACK+"1 requested=true active=false generation=0",
        BRIDGE+"2 requested=false active=false activeRows=0 frames=10",
        ACK+"2 requested=false active=false generation=0",
        DISABLE,
        BRIDGE+"3 requested=true active=false activeRows=0 frames=13",
        ACK+"3 requested=true active=false generation=0",
        REENABLE,CLOSED,OWNER_CLOSE,RESTORE,SHUTDOWN,EXIT])
    validate(good)
    mutants=[
        good.replace(VERSION,"Minecraft 1.12"),
        good.replace("R212GuiOffline","NotOffline"),
        good.replace("slinkyguiinputproof","untrustedmod"),
        good.replace(INITIAL,"missing initial"),
        good.replace("id=1 requested=true","id=1 requested=false"),
        good.replace("id=2 requested=false","id=2 requested=true"),
        good.replace("id=3 requested=true","id=3 requested=false"),
        good.replace("active=false","active=true"),
        good.replace("activeRows=0","activeRows=1"),
        good.replace("frames=7","frames=0"),
        good.replace("frames=10","frames=7"),
        good.replace("frames=13","frames=10"),
        good.replace(CLOSED,"unclosed"),
        good.replace(OWNER_CLOSE,"no native close"),
        good.replace(RESTORE,"not restored"),
        good.replace(EXIT,"System.exit(0)"),
        good+"\n"+BRIDGE+"3 requested=true active=false activeRows=0 frames=15",
        good+"\n"+ACK+"2 requested=false active=false generation=0",
        good.replace(CLOSED+"\n"+OWNER_CLOSE,OWNER_CLOSE+"\n"+CLOSED),
        good+"\nFMLSecurityManager$ExitTrappedException",
    ]
    for i,case in enumerate(mutants):
        try: validate(case)
        except ValueError: continue
        raise AssertionError("R214 negative control accepted: "+str(i))
    print("R214_REAL_WGL_JNI_20_NEGATIVE_CONTROLS_PASS")

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    group=parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--selftest",action="store_true")
    group.add_argument("--log",type=Path)
    args=parser.parse_args()
    if args.selftest:selftest()
    else:
        validate(args.log.read_text(encoding="utf-8",errors="replace"))
        print("R214_REAL_FORGE_WGL_CLICKGUI_PR2_JNI_ARRAYLIST_PASS")
