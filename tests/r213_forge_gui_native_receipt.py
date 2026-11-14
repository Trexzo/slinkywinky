#!/usr/bin/env python3
"""R213 optional real PR2 JNI + Forge-owned GuiScreen acceptance receipt.

The offline negative controls never load a DLL, launch Minecraft, or change
native module state. A live log alone is not cryptographic source attestation.
"""
import argparse
from pathlib import Path

VERSION="Forge Mod Loader version 11.15.1.2318 for Minecraft 1.8.9"
MODE="slinkyguiinputproof"
INITIAL="R213_REAL_NATIVE_SPRINT_INITIAL_OFF_PASS"
OPEN="R212_REAL_FORGE_GUI_SCREEN_INSTALLED"
ACK1="R213_REAL_JNI_NATIVE_SPRINT_ACK id=1 requested=true active=false generation=0"
BUTTON="R212_REAL_FORGE_GUI_BUTTON_ACTION_REQUESTED"
ACK2="R213_REAL_JNI_NATIVE_SPRINT_ACK id=2 requested=false active=false generation=0"
DISABLE="R213_REAL_JNI_SPRINT_DISABLE_AND_RENDER_RECEIPT_PASS"
ACK3="R213_REAL_JNI_NATIVE_SPRINT_ACK id=3 requested=true active=false generation=0"
REENABLE="R213_REAL_JNI_SPRINT_REENABLE_AND_RENDER_RECEIPT_PASS"
CLOSE="R213_REAL_JNI_SPRINT_OWNER_CLOSE_PASS"
ESCAPE="R212_REAL_FORGE_GUI_ESC_RESTORES_PREVIOUS_SCREEN"
DONE="R212_FORGE189_OWN_GUIScreen_MOUSE_AND_ESCAPE_PASS"
SHUTDOWN="R212_ISOLATED_CLIENT_CLEAN_SHUTDOWN"
EXIT="Java has been asked to exit (code 0) by net.minecraftforge.fml.common.FMLCommonHandler.exitJava"
EVENTS=(INITIAL,OPEN,ACK1,BUTTON,ACK2,DISABLE,ACK3,REENABLE,
        CLOSE,ESCAPE,DONE,SHUTDOWN,EXIT)

def validate(log):
    if VERSION not in log or MODE not in log:
        raise ValueError("Wrong Forge/Minecraft version or mod")
    if "R212GuiOffline" not in log:
        raise ValueError("Wrong offline admission identity")
    if "FMLSecurityManager$ExitTrappedException" in log:
        raise ValueError("Trapped exit, not clean")
    locations=[]
    for event in EVENTS:
        if log.count(event)!=1:
            raise ValueError("Missing or repeated receipt: "+event)
        locations.append(log.index(event))
    if locations!=sorted(locations) or len(set(locations))!=len(locations):
        raise ValueError("Incorrect native-ack/display/close ordering")
    all_ack=[line for line in log.splitlines()
             if "R213_REAL_JNI_NATIVE_SPRINT_ACK id=" in line]
    if len(all_ack)!=3:
        raise ValueError("Wrong native-ack count")
    if any("active=true" in line for line in all_ack):
        raise ValueError("No-world active state falsely reported")
    if "R213_REAL_JNI_NATIVE_SPRINT_ACK id=4 " in log:
        raise ValueError("Unacknowledged extra user action")

def selftest():
    good="\n".join((VERSION,"user: R212GuiOffline",MODE)+EVENTS)
    validate(good)
    mutations=[
        good.replace("R212GuiOffline","OnlineUser"),
        good.replace(VERSION,"Minecraft version 1.12"),
        good.replace(MODE,"othermod"),
        good.replace(ACK1,"ack missing"),
        good.replace("id=1 requested=true","id=1 requested=false"),
        good.replace("id=2 requested=false","id=2 requested=true"),
        good.replace("id=3 requested=true","id=3 requested=false"),
        good.replace("active=false","active=true"),
        good.replace(CLOSE,"close missing"),
        good.replace(SHUTDOWN,"shutdown missing"),
        good.replace(EXIT,"System.exit(0)"),
        good+"\n"+ACK2,
        good+"\n"+ACK1,
        good.replace(ACK2+"\n"+DISABLE,DISABLE+"\n"+ACK2),
        good.replace(CLOSE+"\n"+ESCAPE,ESCAPE+"\n"+CLOSE),
        good+"\nFMLSecurityManager$ExitTrappedException",
    ]
    for i,mutated in enumerate(mutations):
        try:
            validate(mutated)
        except ValueError:
            continue
        raise AssertionError("Malformed JNI receipt accepted: "+str(i))
    print("R213_FORGE_NATIVE_RECEIPT_16_NEGATIVE_CONTROLS_PASS")

def main():
    parser=argparse.ArgumentParser()
    arguments=parser.add_mutually_exclusive_group(required=True)
    arguments.add_argument("--selftest",action="store_true")
    arguments.add_argument("--log",type=Path)
    args=parser.parse_args()
    if args.selftest:
        selftest()
    else:
        validate(args.log.read_text(encoding="utf-8",errors="replace"))
        print("R213_REAL_FORGE189_PR2_JNI_THREE_TOGGLE_RECEIPT_PASS")

if __name__=="__main__":
    main()
