#!/usr/bin/env python3
"""R302: verify a real isolated Minecraft 1.8.9 Forge -> JNI ESP receipt.

The selftest checks only synthetic data. A real acceptance requires --root,
--native-jni and the SHA-256 of the exact reviewed Forge build, plus the
separate Windows process-identity wrapper. Never treat log text alone as proof.
"""
import argparse
import hashlib
import re
import tempfile
from pathlib import Path

MC_SHA = "14f0d96d1a56fb4f5c3b2233d00699525893fe5ce3dcf181e7de59120595d298"
JAR_SHA = "b4967fb970e2f369179b8268add1be5a80a8fcd7bba79d69659597213355ebe5"
PREFIX = "slinkywinky-r302-independent-forge-"
MARKERS = (
    "R302_ESP_WORLD_ISOLATION_GATE_PASS",
    "R300_ESP_TEMP_GATE_PASS",
    "R302_LOCAL_WORLD_REQUESTED",
    "R302_LOCAL_WORLD_PLAYER_READY_PASS",
    "R301_REAL_NATIVE_ESP_WORLD_RECEIPT",
    "R300_FORGE_ESP_WORLD_SNAPSHOT_RECEIPT",
    "R302_REAL_FORGE_ESP_NATIVE_JNI_PASS",
    "R302_ESP_NATIVE_CLEANUP_PASS",
)
BAD = (
    "R302_ESP_PREVIEW_START_FAIL",
    "R302_ESP_WORLD_FAIL",
    "R300_ESP_READONLY_FAIL",
    "R301_NATIVE_ESP_READONLY_FAIL",
    "UnsatisfiedLinkError",
    "Exception in thread",
)
COMPLETE = (
    "R302_ESP_NATIVE_CLEANUP_PASS "
    "native_owner_closed=1 preview_closed=1 auto_restart=0 gameWrites=0"
)
RECEIPT = re.compile(
    r"R302_REAL_FORGE_ESP_NATIVE_JNI_PASS "
    r"samples=(\d+) clientTicks=(\d+) "
    r"rawTiles=(\d+) rawDrops=(\d+) rawPlayers=(\d+) "
    r"chestRows=(\d+) itemRows=(\d+) playerRows=(\d+) "
    r"generation=(\d+) gameWrites=0 "
    r"source=Forge_PlayerTick_END_JNI"
)
SHA = re.compile(r"[0-9a-f]{64}")
FOLDER = re.compile(r"slinkywinky-r302-independent-forge-[A-Za-z0-9._-]+")


def single(text, name):
    values = [line[len(name):].strip() for line in text.splitlines()
              if line.startswith(name)]
    if len(values) != 1:
        raise ValueError("Absent or repeated field " + name)
    return values[0]


def check_log(log):
    if any(log.count(marker) != 1 for marker in MARKERS):
        raise ValueError("Missing or repeated R302 causal marker")
    if [log.index(marker) for marker in MARKERS] != sorted(
            log.index(marker) for marker in MARKERS):
        raise ValueError("R302 events occurred out of order")
    if any(bad in log for bad in BAD) or log.count(COMPLETE) != 1:
        raise ValueError("R302 failed or leaked a native observer")
    matches = RECEIPT.findall(log)
    if len(matches) != 1:
        raise ValueError("Missing exact native in-world receipt")
    samples, ticks, tiles, drops, players, chest, item, remote, gen = (
        int(x) for x in matches[0]
    )
    if samples != 32 or ticks < samples or ticks > 1000000 or gen < 1:
        raise ValueError("Incoherent world/tick provenance")
    if not (0 <= tiles <= 4096 and 0 <= drops <= 8192 and
            1 <= players <= 512):
        raise ValueError("Invalid raw world collection cardinality")
    if not (0 <= chest <= min(100, tiles) and
            0 <= item <= min(100, drops) and
            0 <= remote <= min(100, players - 1)):
        raise ValueError("Impossible source ESP controller row counts")
    return samples, ticks, tiles, drops, players, chest, item, remote, gen


def check_preflight(preflight, jar_sha):
    if not SHA.fullmatch(jar_sha) or jar_sha != JAR_SHA:
        raise ValueError("R302 requires exact reviewed GitHub JAR SHA256")
    for marker in ("R207_INDEPENDENT_FORGE_PREFLIGHT_PASS",
                   "PROFILE=ISOLATED_TEMPORARY",
                   "LOGIN=OFFLINE_DUMMY",
                   "ORIGINAL_MINECRAFT_LIBRARIES_AND_ASSETS=READ_ONLY"):
        if preflight.splitlines().count(marker) != 1:
            raise ValueError("Incomplete launcher provenance: " + marker)
    for key, value in (
        ("REOBF_SHA256=", jar_sha),
        ("MC_SHA256=", MC_SHA),
        ("R302_ESP_WORLD_TEST=", "True"),
        ("R302_ESP_NATIVE_READONLY=", "True"),
        ("ISOLATED_WORLD_TEST=", "True"),
        ("R289_ROSTER_WORLD_TEST=", "False"),
        ("R289_ROSTER_NATIVE_READONLY=", "False"),
        ("R224_LOCAL_STRAFE_READONLY=", "False"),
        ("R213_LOCAL_ACTION_ACCEPTANCE=", "False"),
        ("R214_EXPLICIT_POST_SLOW_AGENT=", "False"),
        ("R215_VANILLA_INPUT_TEST=", "False"),
        ("R216_DISPLACEMENT_TEST=", "False"),
        ("R217_ITEM_CATEGORIES_TEST=", "False"),
    ):
        if single(preflight, key) != value:
            raise ValueError("Mismatched test mode/provenance " + key)
    native_sha = single(preflight, "NATIVE_JNI_SHA256=")
    if not SHA.fullmatch(native_sha):
        raise ValueError("No recorded native DLL fingerprint")
    return single(preflight, "STAGED_MOD="), native_sha


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1048576), b""):
            h.update(chunk)
    return h.hexdigest()


def check_real(root, native, jar_sha):
    root = root.resolve(strict=True)
    tmp = Path(tempfile.gettempdir()).resolve(strict=True)
    if not root.is_dir() or root.parent != tmp or not FOLDER.fullmatch(root.name):
        raise ValueError("R302 requires a direct named OS TEMP child")
    jar = root / "mods" / "slinkywinky-preview-r207-reobf.jar"
    preflight = root / "R207_INDEPENDENT_FORGE_PREFLIGHT.txt"
    log = root / "r207-forge-stdout.log"
    world = root / "saves" / "R302-local-only" / "level.dat"
    if any(not p.is_file() for p in (jar, preflight, log, world)):
        raise ValueError("Missing real isolated Forge world evidence")
    recorded, dll_sha = check_preflight(preflight.read_text(
        encoding="utf-8", errors="replace"), jar_sha)
    if Path(recorded).resolve(strict=True) != jar.resolve(strict=True):
        raise ValueError("Staged Forge JAR does not match launcher evidence")
    if sha256(jar) != jar_sha:
        raise ValueError("Incorrect Forge JAR SHA256")
    native = native.resolve(strict=True)
    if native.name.lower() != "slinkywinky_jni.dll" or sha256(native) != dll_sha:
        raise ValueError("Incorrect native JNI DLL SHA256")
    output = log.read_text(encoding="utf-8", errors="replace")
    if ("Forge Mod Loader has identified 4 mods to load" not in output or
            "FMLFileResourcePack:SlinkyWinky Preview" not in output):
        raise ValueError("Actual Minecraft Forge startup evidence missing")
    return check_log(output)


def fixture():
    result = ["[SlinkyWinkyPreview] " + m for m in MARKERS]
    result[4] += (" samples=20 chestRows=0 itemRows=0 playerRows=0 "
                  "generation=1 writes=0 rendering=0")
    result[5] += (" samples=20 tiles=0 drops=0 players=1 generation=1 "
                  "writes=0 cadence=Forge_PlayerTick_END")
    result[6] += (" samples=32 clientTicks=50 rawTiles=0 rawDrops=0 "
                  "rawPlayers=1 chestRows=0 itemRows=0 playerRows=0 "
                  "generation=1 gameWrites=0 source=Forge_PlayerTick_END_JNI")
    result[7] += (" native_owner_closed=1 preview_closed=1 "
                  "auto_restart=0 gameWrites=0")
    return "\n".join(result)


def preflight_fixture():
    lines = ["R207_INDEPENDENT_FORGE_PREFLIGHT_PASS",
             "REOBF_SHA256=" + JAR_SHA,
             "MC_SHA256=" + MC_SHA,
             "R302_ESP_WORLD_TEST=True",
             "R302_ESP_NATIVE_READONLY=True",
             "ISOLATED_WORLD_TEST=True",
             "NATIVE_JNI_SHA256=" + "b" * 64,
             "STAGED_MOD=C:/Temp/isolation/mods/slinkywinky-preview-r207-reobf.jar",
             "PROFILE=ISOLATED_TEMPORARY", "LOGIN=OFFLINE_DUMMY",
             "ORIGINAL_MINECRAFT_LIBRARIES_AND_ASSETS=READ_ONLY"]
    for field in ("R289_ROSTER_WORLD_TEST", "R289_ROSTER_NATIVE_READONLY",
                  "R224_LOCAL_STRAFE_READONLY", "R213_LOCAL_ACTION_ACCEPTANCE",
                  "R214_EXPLICIT_POST_SLOW_AGENT", "R215_VANILLA_INPUT_TEST",
                  "R216_DISPLACEMENT_TEST", "R217_ITEM_CATEGORIES_TEST"):
        lines.append(field + "=False")
    return "\n".join(lines)


def refuses(func):
    try:
        func()
    except ValueError:
        return
    raise AssertionError("R302 accepted forged or incomplete evidence")


def selftest():
    good = fixture()
    check_log(good)
    p = preflight_fixture()
    check_preflight(p, JAR_SHA)
    for marker in MARKERS:
        refuses(lambda marker=marker: check_log(good.replace(marker, "ABSENT", 1)))
    for old, new in (
        ("samples=32", "samples=31"), ("clientTicks=50", "clientTicks=20"),
        ("rawPlayers=1", "rawPlayers=0"), ("chestRows=0", "chestRows=101"),
        ("playerRows=0", "playerRows=1"),
        ("generation=1 gameWrites=0", "generation=0 gameWrites=0"),
        ("native_owner_closed=1", "native_owner_closed=0"),
    ):
        refuses(lambda old=old, new=new: check_log(good.replace(old, new)))
    refuses(lambda: check_log(good + "\n" + good))
    refuses(lambda: check_log(good + "\nR302_ESP_WORLD_FAIL"))
    refuses(lambda: check_preflight(p.replace("R302_ESP_WORLD_TEST=True",
                                               "R302_ESP_WORLD_TEST=False"), "a" * 64))
    refuses(lambda: check_preflight(p, "bad-sha"))
    print("R302_ESP_WORLD_RECEIPT_NEGATIVE_CONTROLS_PASS")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--selftest", action="store_true")
    parser.add_argument("--root", type=Path)
    parser.add_argument("--native-jni", type=Path)
    parser.add_argument("--expected-jar-sha256")
    options = parser.parse_args()
    if options.selftest:
        selftest()
    elif options.root and options.native_jni and options.expected_jar_sha256:
        result = check_real(options.root, options.native_jni,
                            options.expected_jar_sha256.lower())
        print("R302_REAL_FORGE_ESP_RECEIPT_PROVENANCE_PASS "
              "samples={} clientTicks={} rawTiles={} rawDrops={} rawPlayers={} "
              "chestRows={} itemRows={} playerRows={} generation={}".format(*result))
    else:
        parser.error("R302 requires --selftest OR exact --root, --native-jni and --expected-jar-sha256")
