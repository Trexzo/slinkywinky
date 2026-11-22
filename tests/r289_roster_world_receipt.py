#!/usr/bin/env python3
"""R291: verify only a SHA-pinned isolated local Forge R289 world receipt.

--selftest uses synthetic log/preflight text and negative controls ONLY.
--root and --native-jni require real files; a bare text log cannot be
presented as a verified Minecraft process/world acceptance.
"""
import argparse
import hashlib
import re
import tempfile
from pathlib import Path

JAR_SHA = "37ed0d9ec4319ae7ffeefce19787394d6de1d75214c27f063b85c461ea62a807"
MC_SHA = "14f0d96d1a56fb4f5c3b2233d00699525893fe5ce3dcf181e7de59120595d298"
PREFIX = "slinkywinky-r278-independent-forge-"
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
    r"source=Forge_PlayerTick_END_JNI"
)
CLEAN = (
    "R289_ROSTER_NATIVE_CLEANUP_PASS "
    "native_owner_closed=1 preview_closed=1 "
    "auto_restart=0 gameWrites=0"
)
BAD = (
    "R289_PREVIEW_START_FAIL", "R289_ROSTER_WORLD_FAIL",
    "R278_ROSTER_READONLY_FAIL", "R283_NATIVE_ROSTER_READONLY_FAIL",
    "UnsatisfiedLinkError", "Exception in thread",
)
EXCLUSIVE_OFF = (
    "PLAYER_TICK_TEST", "R224_LOCAL_STRAFE_READONLY",
    "R213_LOCAL_ACTION_ACCEPTANCE", "R214_EXPLICIT_POST_SLOW_AGENT",
    "R215_VANILLA_INPUT_TEST", "R216_DISPLACEMENT_TEST",
    "R217_ITEM_CATEGORIES_TEST",
)


def validate_log(log):
    if any(log.count(m) != 1 for m in MARKERS):
        raise ValueError("Missing or repeated player-world/JNI marker")
    positions = [log.index(m) for m in MARKERS]
    if positions != sorted(positions):
        raise ValueError("World/JNI/cleanup causal order invalid")
    if any(bad in log for bad in BAD) or log.count(CLEAN) != 1:
        raise ValueError("Native JNI failure or incomplete owner shutdown")
    records = RECEIPT.findall(log)
    if len(records) != 1:
        raise ValueError("Missing complete native player-tick receipt")
    samples, ticks, remote, generation = map(int, records[0])
    if samples != 32 or not 32 <= ticks <= 10000 or not 0 <= remote <= 256 or generation < 1:
        raise ValueError("Implausible roster and client-tick counters")
    return samples, ticks, remote, generation


def one_line(preflight, key):
    lines = [line[len(key):] for line in preflight.splitlines() if line.startswith(key)]
    if len(lines) != 1:
        raise ValueError("Missing or repeated preflight field " + key)
    return lines[0].strip()


def validate_preflight(preflight):
    for marker in (
        "R207_INDEPENDENT_FORGE_PREFLIGHT_PASS",
        "PROFILE=ISOLATED_TEMPORARY",
        "LOGIN=OFFLINE_DUMMY",
        "ORIGINAL_MINECRAFT_LIBRARIES_AND_ASSETS=READ_ONLY",
    ):
        if preflight.splitlines().count(marker) != 1:
            raise ValueError("Missing exact isolated launcher marker " + marker)
    expected = {
        "REOBF_SHA256=": JAR_SHA,
        "MC_SHA256=": MC_SHA,
        "R289_ROSTER_WORLD_TEST=": "True",
        "R289_ROSTER_NATIVE_READONLY=": "True",
        "ISOLATED_WORLD_TEST=": "True",
    }
    for key, value in expected.items():
        if one_line(preflight, key) != value:
            raise ValueError("Wrong launcher provenance for " + key)
    for field in EXCLUSIVE_OFF:
        if one_line(preflight, field + "=") != "False":
            raise ValueError("R289 mixed with another acceptance mode: " + field)
    native_sha = one_line(preflight, "NATIVE_JNI_SHA256=")
    if not re.fullmatch(r"[0-9a-f]{64}", native_sha):
        raise ValueError("Missing exact native DLL SHA256")
    staged = one_line(preflight, "STAGED_MOD=")
    if not staged:
        raise ValueError("No staged Forge JAR")
    return staged, native_sha


def sha256_file(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def validate_root(root, native_jni):
    # A supplied arbitrary log is insufficient: the genuine gameDir must
    # be an immediate, existing child of the canonical OS TEMP root.
    try:
        root = root.resolve(strict=True)
        temp = Path(tempfile.gettempdir()).resolve(strict=True)
    except (OSError, RuntimeError) as exc:
        raise ValueError("R289 missing canonical TEMP world") from exc
    if not root.is_dir() or root.parent != temp or not root.name.startswith(PREFIX) \
            or len(root.name) <= len(PREFIX):
        raise ValueError("R289 refuses non-TEMP, nested or unscoped world")
    staged = root / "mods" / "slinkywinky-preview-r207-reobf.jar"
    preflight = root / "R207_INDEPENDENT_FORGE_PREFLIGHT.txt"
    log = root / "r207-forge-stdout.log"
    level = root / "saves" / "R289-local-only" / "level.dat"
    for required in (staged, preflight, log, level):
        if not required.is_file():
            raise ValueError("R289 missing isolated world input: " + str(required))
    recorded, recorded_native = validate_preflight(
        preflight.read_text(encoding="utf-8", errors="replace"))
    if Path(recorded).resolve(strict=True) != staged.resolve(strict=True):
        raise ValueError("R289 staged mod path does not match preflight")
    if sha256_file(staged) != JAR_SHA:
        raise ValueError("R289 reobfuscated source JAR hash mismatch")
    if native_jni.name.lower() != "slinkywinky_jni.dll" or \
            not native_jni.is_file() or sha256_file(native_jni) != recorded_native:
        raise ValueError("R289 native JNI DLL absent or hash mismatch")
    output = log.read_text(encoding="utf-8", errors="replace")
    if "Forge Mod Loader has identified 4 mods to load" not in output \
            or "FMLFileResourcePack:SlinkyWinky Preview" not in output:
        raise ValueError("R289 genuine Forge loader evidence absent")
    return validate_log(output)


def fixture():
    return "\n".join((
        "[SlinkyWinkyPreview] " + MARKERS[0],
        "[SlinkyWinkyPreview] " + MARKERS[1],
        "[SlinkyWinkyPreview] " + MARKERS[2],
        "[SlinkyWinkyPreview] " + MARKERS[3],
        "[SlinkyWinkyPreview] R283_FORGE_NATIVE_ANTIBOT_READONLY_RECEIPT "
        "samples=20 remoteCount=0 rejectedCount=0 generation=1 "
        "proposalsOnly=1 gameWrites=0 source=Forge_PlayerTick_END_JNI",
        "[SlinkyWinkyPreview] R289_REAL_WORLD_ANTIBOT_NATIVE_JNI_PASS "
        "samples=32 clientTicks=48 remoteCount=0 generation=1 "
        "gameWrites=0 source=Forge_PlayerTick_END_JNI",
        "[SlinkyWinkyPreview] " + CLEAN,
    ))


def preflight_fixture():
    fields = (
        "R207_INDEPENDENT_FORGE_PREFLIGHT_PASS",
        "REOBF_SHA256=" + JAR_SHA,
        "MC_SHA256=" + MC_SHA,
        "ISOLATED_WORLD_TEST=True",
        "R289_ROSTER_WORLD_TEST=True",
        "R289_ROSTER_NATIVE_READONLY=True",
        "NATIVE_JNI_SHA256=" + "a" * 64,
        "STAGED_MOD=C:\\Temp\\staged.jar",
        "PROFILE=ISOLATED_TEMPORARY",
        "LOGIN=OFFLINE_DUMMY",
        "ORIGINAL_MINECRAFT_LIBRARIES_AND_ASSETS=READ_ONLY",
    )
    return "\n".join(fields + tuple(x + "=False" for x in EXCLUSIVE_OFF))


def denied(action):
    try:
        action()
    except ValueError:
        return
    raise AssertionError("R291 accepted invalid/fabricated provenance")


def selftest():
    good = fixture()
    validate_log(good)
    fake_preflight = preflight_fixture()
    validate_preflight(fake_preflight)
    mutants = [good.replace(marker, "ABSENT", 1) for marker in MARKERS]
    mutants.extend((
        good.replace("clientTicks=48", "clientTicks=1"),
        good.replace("samples=32", "samples=31"),
        good.replace("remoteCount=0 generation=1 gameWrites=0",
                     "remoteCount=999 generation=1 gameWrites=0"),
        good.replace("gameWrites=0 source=Forge_PlayerTick_END_JNI",
                     "gameWrites=1 source=Forge_PlayerTick_END_JNI"),
        good.replace("generation=1 gameWrites=0", "generation=0 gameWrites=0"),
        good.replace("native_owner_closed=1", "native_owner_closed=0"),
        good.replace("auto_restart=0", "auto_restart=1"),
        good + "\n" + MARKERS[5],
        good + "\nR289_ROSTER_WORLD_FAIL synthetic",
        good.replace(MARKERS[3], MARKERS[2]).replace(
            MARKERS[2], MARKERS[3], 1),
    ))
    for mutant in mutants:
        denied(lambda candidate=mutant: validate_log(candidate))
    tampered = [
        fake_preflight.replace(JAR_SHA, "0" * 64),
        fake_preflight.replace(MC_SHA, "0" * 64),
        fake_preflight.replace("R289_ROSTER_WORLD_TEST=True",
                               "R289_ROSTER_WORLD_TEST=False"),
        fake_preflight.replace("R289_ROSTER_NATIVE_READONLY=True",
                               "R289_ROSTER_NATIVE_READONLY=False"),
        fake_preflight.replace("NATIVE_JNI_SHA256=" + "a" * 64,
                               "NATIVE_JNI_SHA256=bad"),
        fake_preflight.replace("PLAYER_TICK_TEST=False", "PLAYER_TICK_TEST=True"),
        fake_preflight.replace("LOGIN=OFFLINE_DUMMY", "LOGIN=ONLINE"),
        fake_preflight.replace("STAGED_MOD=C:\\Temp\\staged.jar", "STAGED_MOD="),
    ]
    for mutant in tampered:
        denied(lambda candidate=mutant: validate_preflight(candidate))
    with tempfile.TemporaryDirectory(prefix="r291-bad-root-") as tmp:
        denied(lambda: validate_root(Path(tmp), Path(tmp) / "missing.dll"))
    print("R291_ROSTER_RECEIPT_%d_PROVENANCE_NEGATIVE_CONTROLS_PASS" %
          (len(mutants) + len(tampered) + 1))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--selftest", action="store_true")
    mode.add_argument("--root", type=Path)
    parser.add_argument("--native-jni", type=Path)
    args = parser.parse_args()
    if args.selftest:
        selftest()
    else:
        if args.native_jni is None:
            parser.error("--root requires --native-jni")
        values = validate_root(args.root, args.native_jni)
        print("R291_REAL_FORGE_ROSTER_RECEIPT_PROVENANCE_PASS "
              "samples=%d ticks=%d remote=%d generation=%d" % values)
