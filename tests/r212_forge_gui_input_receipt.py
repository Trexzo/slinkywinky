#!/usr/bin/env python3
"""R212 isolated Forge 1.8.9 GuiScreen acceptance receipt.

Checks logs or selftests; never launches Minecraft, sends input, reads
secrets, changes game state, or treats a scripted click as physical input.
"""
import argparse
from pathlib import Path

MARKERS = (
    "R212_REAL_FORGE_GUI_SCREEN_INSTALLED",
    "R212_REAL_FORGE_GUI_BUTTON_ACTION_REQUESTED",
    "R212_REAL_FORGE_GUI_ESC_RESTORES_PREVIOUS_SCREEN",
    "R212_FORGE189_OWN_GUIScreen_MOUSE_AND_ESCAPE_PASS",
    "R212_ISOLATED_CLIENT_CLEAN_SHUTDOWN",
)
REQUEST = (
    "[R212GuiProof] request-only canonical=Sprint "
    "id=1 requested=true active=false"
)
VERSION = "Forge Mod Loader version 11.15.1.2318 for Minecraft 1.8.9"
EXIT = "Java has been asked to exit (code 0) by net.minecraftforge.fml.common.FMLCommonHandler.exitJava"


def validate(log: str) -> None:
    if VERSION not in log:
        raise ValueError("Missing exact Minecraft/Forge 1.8.9 version")
    if "slinkyguiinputproof" not in log:
        raise ValueError("Missing first-party input proof mod")
    if log.count(REQUEST) != 1:
        raise ValueError("Sprint request-only receipt must appear exactly once")
    if EXIT not in log:
        raise ValueError("Missing supported Forge clean exit receipt")
    if "FMLSecurityManager$ExitTrappedException" in log:
        raise ValueError("Unsafe or trapped JVM exit")
    if "R212_REAL_FORGE_GUI" not in log:
        raise ValueError("Missing live Forge GUI receipts")
    locations = []
    for marker in MARKERS:
        if log.count(marker) != 1:
            raise ValueError("Missing or duplicate GUI event: " + marker)
        locations.append(log.index(marker))
    request_at = log.index(REQUEST)
    if not (
        locations[0] < request_at < locations[1] < locations[2]
        < locations[3] < locations[4] < log.index(EXIT)
    ):
        raise ValueError("Invalid GUI input/restore/exit event ordering")


def selftest() -> None:
    lines = [
        VERSION,
        "mods loaded: slinkyguiinputproof",
        MARKERS[0],
        REQUEST,
        MARKERS[1],
        MARKERS[2],
        MARKERS[3],
        MARKERS[4],
        EXIT,
    ]
    good = "\n".join(lines)
    validate(good)
    mutants = [
        good.replace("requested=true", "requested=false"),
        good.replace("active=false", "active=true"),
        good.replace(VERSION, "Minecraft 1.12"),
        good.replace("slinkyguiinputproof", "anothermod"),
        good.replace(MARKERS[1], "wrong button"),
        good.replace(MARKERS[2], "wrong close"),
        good.replace(MARKERS[4], "no exit"),
        good.replace(EXIT, "java.lang.System.exit"),
        good + "\n" + MARKERS[1],
        good + "\n" + REQUEST,
        good.replace(
            MARKERS[1] + "\n" + MARKERS[2],
            MARKERS[2] + "\n" + MARKERS[1],
        ),
        good + "\nFMLSecurityManager$ExitTrappedException",
    ]
    for index, case in enumerate(mutants):
        try:
            validate(case)
        except ValueError:
            continue
        raise AssertionError("Negative control passed: " + str(index))
    print("R212_FORGE_GUI_RECEIPT_12_NEGATIVE_CONTROLS_PASS")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--selftest", action="store_true")
    group.add_argument("--log", type=Path)
    args = parser.parse_args()
    if args.selftest:
        selftest()
    else:
        text = args.log.read_text(encoding="utf-8", errors="replace")
        validate(text)
        print("R212_REAL_FORGE189_GUI_INPUT_RECEIPT_VALIDATED")


if __name__ == "__main__":
    main()
