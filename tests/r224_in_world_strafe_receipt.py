#!/usr/bin/env python3
"""R224 independent real Forge 1.8.9 native Strafe observation receipt.

Validates local hash-pinned launcher, actual TEMP-only world and JNI callback
receipts. CI --selftest uses synthetic strings; it is NOT Minecraft execution.
"""
import argparse
import hashlib
import math
import re
import tempfile
from pathlib import Path

JAR_SHA = "ba8759cec99148cf14b31ad4e4b2c7ec06abe387a9730e3a0bb8586d60b2d345"
VANILLA_SHA = "14f0d96d1a56fb4f5c3b2233d00699525893fe5ce3dcf181e7de59120595d298"
MARKERS = (
    "R224_STRAFE_ISOLATION_GATE_PASS",
    "R224_LOCAL_WORLD_REQUESTED",
    "R224_LOCAL_WORLD_PLAYER_READY_PASS",
    "R224_REAL_VANILLA_INPUT_ARMED source=MovementInputFromOptions synthetic_key=forward",
    "R224_REAL_PLAYER_TICK_READONLY_STRAFE_JNI_PASS",
    "R224_READONLY_STRAFE_CLEANUP_PASS",
)
METRICS = re.compile(
    r"R224_REAL_PLAYER_TICK_READONLY_STRAFE_JNI_PASS "
    r"samples=(\d+) movingSamples=(\d+) proposedSamples=(\d+) "
    r"distance=([0-9]+(?:\.[0-9]+)?) gameMotionWrites=0 "
    r"source=Forge_PlayerTick_END_and_vanilla_motion"
)
CLEAN = (
    "R224_READONLY_STRAFE_CLEANUP_PASS key_released=1 "
    "native_owner_closed=1 preview_closed=1 auto_restart=0 "
    "gameplay_motion_writes=0"
)
PROHIBITED = (
    "R224_READONLY_STRAFE_FAIL",
    "R208_PREVIEW_START_FAIL",
    "R209_PLAYER_TICK_FAIL",
    "Exception in thread",
    "UnsupportedClassVersionError",
    "NoSuchMethodError",
    "UnsatisfiedLinkError",
)


def validate_receipt(log: str, preflight: str) -> tuple[int, int, int, float]:
    requirements = (
        "R207_INDEPENDENT_FORGE_PREFLIGHT_PASS",
        "REOBF_SHA256=" + JAR_SHA,
        "MC_SHA256=" + VANILLA_SHA,
        "ISOLATED_WORLD_TEST=True",
        "R224_LOCAL_STRAFE_READONLY=True",
        "PROFILE=ISOLATED_TEMPORARY",
        "LOGIN=OFFLINE_DUMMY",
        "ORIGINAL_MINECRAFT_LIBRARIES_AND_ASSETS=READ_ONLY",
    )
    if any(preflight.count(p) != 1 for p in requirements):
        raise ValueError("R224 hash/isolation/identity launcher proof missing")
    if any(log.count(marker) != 1 for marker in MARKERS):
        raise ValueError("R224 required actual Forge marker absent or repeated")
    if any(word in log for word in PROHIBITED):
        raise ValueError("R224 exception, wrong JNI ABI or launch failure")
    positions = [log.index(marker) for marker in MARKERS]
    if positions != sorted(positions) or len(set(positions)) != len(positions):
        raise ValueError("R224 world/observer/cleanup causal order invalid")
    if log.count(CLEAN) != 1:
        raise ValueError("R224 cleanup did not retire native owner/key")
    matches = METRICS.findall(log)
    if len(matches) != 1:
        raise ValueError("R224 invalid JNI movement and displacement receipt")
    samples, moving, proposed = map(int, matches[0][:3])
    distance = float(matches[0][3])
    if (samples < 50 or samples > 150 or moving < 10 or
        proposed < 10 or moving > samples or proposed > samples or
        not math.isfinite(distance) or distance <= 0.1 or distance > 100):
        raise ValueError("R224 implausible observed motion/proposal counts")
    if log.count("R224_REAL_VANILLA_INPUT_ARMED") != 1:
        raise ValueError("R224 test input rearmed after cleanup")
    return samples, moving, proposed, distance


def validate_path(root: Path):
    folder = root.resolve()
    temp = Path(tempfile.gettempdir()).resolve()
    if (folder.parent != temp or
        not folder.name.startswith("slinkywinky-r224-independent-forge-")):
        raise ValueError("R224 verifier refuses non-TEMP game directory")
    preflight = (folder / "R207_INDEPENDENT_FORGE_PREFLIGHT.txt").read_text(
        encoding="utf-8")
    staged = folder / "mods" / "slinkywinky-preview-r207-reobf.jar"
    actual_sha = hashlib.sha256(staged.read_bytes()).hexdigest()
    if actual_sha != JAR_SHA:
        raise ValueError("R224 loaded JAR does not match built/reobfuscated source")
    if not (folder / "saves" / "R224-local-only" / "level.dat").is_file():
        raise ValueError("R224 local integrated world save never materialized")
    log = (folder / "r207-forge-stdout.log").read_text(
        encoding="utf-8", errors="replace")
    values = validate_receipt(log, preflight)
    print("R224_REAL_FORGE_WORLD_STRAFE_JNI_RECEIPT_PASS "
          f"samples={values[0]} movingSamples={values[1]} "
          f"proposals={values[2]} distance={values[3]:.9f} "
          "native_motion_writes=0")


def fixture():
    preflight = "\n".join((
        "R207_INDEPENDENT_FORGE_PREFLIGHT_PASS",
        "REOBF_SHA256=" + JAR_SHA,
        "MC_SHA256=" + VANILLA_SHA,
        "ISOLATED_WORLD_TEST=True",
        "R224_LOCAL_STRAFE_READONLY=True",
        "PROFILE=ISOLATED_TEMPORARY",
        "LOGIN=OFFLINE_DUMMY",
        "ORIGINAL_MINECRAFT_LIBRARIES_AND_ASSETS=READ_ONLY",
    ))
    output = "\n".join((
        MARKERS[0], MARKERS[1], MARKERS[2], MARKERS[3],
        "R224_REAL_PLAYER_TICK_READONLY_STRAFE_JNI_PASS "
        "samples=50 movingSamples=50 proposedSamples=50 "
        "distance=10.233705335225267 gameMotionWrites=0 "
        "source=Forge_PlayerTick_END_and_vanilla_motion",
        CLEAN,
    ))
    return output, preflight


def selftest():
    good, preflight = fixture()
    validate_receipt(good, preflight)
    mutants = [
        (good.replace(MARKERS[0], "ISOLATION_FAILED"), preflight),
        (good.replace(MARKERS[1], "WORLD_NOT_REQUESTED"), preflight),
        (good.replace(MARKERS[2], "NO_PLAYER"), preflight),
        (good.replace(MARKERS[3], "FAKE_VANILLA_INPUT"), preflight),
        (good.replace("movingSamples=50", "movingSamples=0"), preflight),
        (good.replace("movingSamples=50", "movingSamples=51"), preflight),
        (good.replace("proposedSamples=50", "proposedSamples=0"), preflight),
        (good.replace("proposedSamples=50", "proposedSamples=51"), preflight),
        (good.replace("samples=50", "samples=4"), preflight),
        (good.replace("distance=10.233705335225267", "distance=0.0"), preflight),
        (good.replace("distance=10.233705335225267", "distance=999"), preflight),
        (good.replace("gameMotionWrites=0", "gameMotionWrites=1"), preflight),
        (good.replace("native_owner_closed=1", "native_owner_closed=0"), preflight),
        (good.replace("key_released=1", "key_released=0"), preflight),
        (good.replace("auto_restart=0", "auto_restart=1"), preflight),
        (good + "\n" + CLEAN, preflight),
        (good + "\nR224_READONLY_STRAFE_FAIL JNI", preflight),
        (good.replace(MARKERS[3] + "\n", ""), preflight),
        (good.replace(MARKERS[1] + "\n" + MARKERS[2],
                      MARKERS[2] + "\n" + MARKERS[1]), preflight),
        (good, preflight.replace("R224_LOCAL_STRAFE_READONLY=True",
                                 "R224_LOCAL_STRAFE_READONLY=False")),
        (good, preflight.replace(JAR_SHA, "0" * 64)),
        (good, preflight.replace(VANILLA_SHA, "0" * 64)),
        (good, preflight.replace("LOGIN=OFFLINE_DUMMY", "LOGIN=ONLINE")),
    ]
    for i, (bad, proof) in enumerate(mutants):
        try:
            validate_receipt(bad, proof)
        except ValueError:
            continue
        raise AssertionError("R224 fake receipt accepted: " + str(i))
    print(f"R224_STRAFE_RECEIPT_{len(mutants)}_NEGATIVE_CONTROLS_PASS")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--selftest", action="store_true")
    mode.add_argument("--root", type=Path)
    args = parser.parse_args()
    if args.selftest:
        selftest()
    else:
        validate_path(args.root)
