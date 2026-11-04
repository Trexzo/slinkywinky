#!/usr/bin/env python3
"""R81 operator-only original vanilla 1.8.9 render acceptance receipt checker.

Never launches Minecraft, discovers arbitrary processes, modifies source game
files or attaches by default. --execute is an explicit user action. A target
JVM log containing Java-agent System.out (NOT just latest.log by assumption)
must be provided. Only fresh appended bytes count as receipts.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

PREFLIGHT = "R73_LOADED_ORIGINAL_VANILLA_189_RENDERER_PREFLIGHT_PASS"
RESTORED = "R75_ORIGINAL_RENDER_CLASS_RESTORE_SHA256_PASS"
CALLBACK = "R75_RENDER_CALLBACK_BOUNDED_PASS accepted=8"
RELEASED = "R81_ORIGINAL_RENDER_ADMISSION_RELEASED_PASS"
STABLE = ("R78_READ_ONLY_WGL_CONTEXT_STABLE_PASS "
          "attempts=8 current=8 absent=0 invalid=0 changed=0 stable=true")
NEGATIVE = (
    "R75_RENDER_RESTORE_FAIL",
    "R75_RESTORE_UNCERTAIN_NO_REATTACH",
    "R75_RENDER_CALLBACK_UNVERIFIED",
    "R78_READ_ONLY_WGL_CONTEXT_UNVERIFIED",
    "Exception in thread",
    "NoClassDefFoundError",
)
STAGES = {
    "preflight": (("--render-preflight", (PREFLIGHT,)),),
    "callback": (("--render-preflight", (PREFLIGHT,)),
                 ("--render-callback=8", (RESTORED, CALLBACK, RELEASED))),
    "context": (("--render-preflight", (PREFLIGHT,)),
                ("--render-callback=8", (RESTORED, CALLBACK, RELEASED)),
                ("--render-context=8", (RESTORED, CALLBACK, STABLE, RELEASED))),
}


class ReceiptError(RuntimeError):
    pass


def validate_receipt(option, appended):
    """Require one fresh target-side marker for EACH required receipt."""
    expected = dict(STAGES["context"])[option]
    for bad in NEGATIVE:
        if bad in appended:
            raise ReceiptError("Negative target marker present: " + bad)
    for marker in expected:
        if appended.count(marker) != 1:
            raise ReceiptError("Expected exactly one new target marker: " + marker)
    # A cross-stage callback or stale preflight receipt in this fresh span is
    # ambiguous; never mistake overlapping asynchronous sessions for proof.
    for marker in (PREFLIGHT, RESTORED, CALLBACK, STABLE, RELEASED):
        if marker not in expected and marker in appended:
            raise ReceiptError("Unexpected diagnostic stage marker: " + marker)
    # The post-release marker is emitted only after the agent actually
    # releases the shared instrumentation gate. Order is part of the receipt.
    if option != "--render-preflight":
        ordered = (RESTORED, CALLBACK, STABLE, RELEASED) if (
            option == "--render-context=8") else (RESTORED, CALLBACK, RELEASED)
        positions = [appended.index(marker) for marker in ordered]
        if positions != sorted(positions):
            raise ReceiptError("Renderer restoration / release receipts out of order")
    if option == "--render-context=8":
        matches = re.findall(r"R78_READ_ONLY_WGL_CONTEXT_(?:STABLE_PASS|UNVERIFIED)", appended)
        if len(matches) != 1:
            raise ReceiptError("Ambiguous R78 context evidence")
    if "ATTACH_RESULT=SUCCESS" in appended:
        # This marker is from the EXTERNAL launcher, not evidence of game state.
        raise ReceiptError("Target log contaminated by external loader receipt")
    return tuple(expected)


def read_appended(path, offset, original_stat=None):
    current = path.stat()
    if original_stat is not None and (
            current.st_dev != original_stat.st_dev or
            current.st_ino != original_stat.st_ino):
        raise ReceiptError("Target log replaced during acceptance")
    size = current.st_size
    if size < offset:
        raise ReceiptError("Target log rotated or truncated; cannot use stale receipts")
    with path.open("rb") as stream:
        stream.seek(offset)
        return stream.read().decode("utf-8", errors="replace")


def check_target_listing(stdout, pid):
    """Require one unambiguous already-running vanilla main-class candidate."""
    rows = []
    for line in stdout.splitlines():
        candidate = re.match(r"^([1-9][0-9]*)\s+(.+?)\s*$", line)
        if candidate:
            main = candidate.group(2)
            if main.startswith("net.minecraft.client.main.Main") and (
                    len(main) == len("net.minecraft.client.main.Main") or
                    main[len("net.minecraft.client.main.Main")].isspace() or
                    main[len("net.minecraft.client.main.Main")] == "["):
                rows.append((candidate.group(1), main))
    # Reject an additional Gradle/Forge JVM even when vanilla PID matches.
    # Reuse the same tested PID/descriptor extraction as vanilla candidates.
    other_game = []
    for line in stdout.splitlines():
        candidate = re.match(r"^([1-9][0-9]*)\s+(.+?)\s*$", line)
        if candidate and (
                "net.minecraft.launchwrapper.Launch" in candidate.group(2) or
                "GradleStart" in candidate.group(2)):
            other_game.append(candidate.group(1))
    if other_game or len(rows) != 1 or rows[0][0] != str(pid):
        raise ReceiptError("Expected exactly one isolated vanilla main JVM matching PID")
    return rows[0][1]


def require_file(path, name):
    if not path.is_file():
        raise ReceiptError(name + " not found: " + str(path))


def wait_new_receipt(log_path, before, original_stat, option, timeout):
    deadline = time.monotonic() + timeout
    last_error = "No new target JVM evidence yet"
    while time.monotonic() < deadline:
        current = read_appended(log_path, before, original_stat)
        try:
            return validate_receipt(option, current)
        except ReceiptError as error:
            last_error = str(error)
            # A detected negative result must stop immediately.
            if any(bad in current for bad in NEGATIVE) or (
                    "Target log contaminated" in last_error):
                raise
        time.sleep(.2)
    raise ReceiptError("Target evidence incomplete: " + last_error)


def run(argv=None):
    p = argparse.ArgumentParser(
        description="Manual exact original 1.8.9 read-only render acceptance")
    p.add_argument("--loader", required=True, type=Path)
    p.add_argument("--jdk8", required=True, type=Path)
    p.add_argument("--pid", required=True, type=int)
    p.add_argument("--target-stdout-log", required=True, type=Path)
    p.add_argument("--mode", choices=sorted(STAGES), default="context")
    p.add_argument("--timeout-seconds", type=int, default=30)
    p.add_argument("--execute", action="store_true",
                   help="Explicitly authorize Attach against already-running vanilla JVM")
    p.add_argument("--report", type=Path,
                   help="Write local sanitized evidence JSON (no game files/pointers)")
    a = p.parse_args(argv)
    if not 1 <= a.pid <= 4294967295:
        raise ReceiptError("Invalid PID")
    if not 15 <= a.timeout_seconds <= 60:
        raise ReceiptError("Timeout must be between 15 and 60 seconds")
    steps = STAGES[a.mode]
    if not a.execute:
        print("R81_PLAN_ONLY_NO_ATTACH")
        for option, receipts in steps:
            print(option + " -> " + ", ".join(receipts))
        print("REQUIRES_EXPLICIT_EXECUTE_AND_ALREADY_RUNNING_ORIGINAL_VANILLA")
        return 0

    if os.name != "nt":
        raise ReceiptError("Live original-189 acceptance is Windows-only")
    loader = a.loader.resolve(strict=True)
    jdk8 = a.jdk8.resolve(strict=True)
    target_log = a.target_stdout_log.resolve(strict=True)
    require_file(loader, "Native loader")
    require_file(jdk8 / "bin" / "java.exe", "Java 8 runtime")
    require_file(jdk8 / "lib" / "tools.jar", "Full JDK8 Attach API")
    require_file(target_log, "Target JVM stdout log")
    if a.report is not None and a.report.resolve().exists():
        raise ReceiptError("Refusing to overwrite an existing evidence report")
    if loader.name.lower() != "slinkywinky_loader.exe":
        raise ReceiptError("Exact native loader executable required")

    # Do NOT allow a synthetic fixture bypass or an extra gameplay hook gate.
    env = os.environ.copy()
    env.pop("SLINKYWINKY_ATTACH_TEST", None)
    env.pop("SLINKYWINKY_EXPERIMENTAL_HOOK", None)
    env["SLINKYWINKY_RENDER_PREFLIGHT"] = "1"
    env["SLINKYWINKY_RENDER_CALLBACK"] = "1"
    env["SLINKYWINKY_RENDER_CONTEXT"] = "1"

    listed = subprocess.run([str(loader), "list", str(jdk8)],
                            capture_output=True, text=True, env=env,
                            timeout=15, check=True)
    check_target_listing(listed.stdout, a.pid)
    records = []
    for option, _required in steps:
        # A second check prevents a vanished/replaced PID being attached.
        current = subprocess.run([str(loader), "list", str(jdk8)],
                                 capture_output=True, text=True, env=env,
                                 timeout=15, check=True)
        check_target_listing(current.stdout, a.pid)
        original_stat = target_log.stat()
        before = original_stat.st_size
        cmd = [str(loader), "attach", str(a.pid), str(jdk8), option]
        result = subprocess.run(cmd,capture_output=True,text=True,
                                env=env,timeout=25)
        if result.returncode != 0 or (
                "ATTACH_RESULT=SUCCESS pid="+str(a.pid)) not in result.stdout:
            raise ReceiptError("External Attach did not authorize "+option+
                               "; STOP, do not try another diagnostic")
        receipts = wait_new_receipt(
            target_log,before,original_stat,option,a.timeout_seconds)
        records.append({"mode":option,"target_receipts":list(receipts)})
        print("R81_FRESH_TARGET_RECEIPTS_PASS mode="+option,flush=True)

    # Receipt success is insufficient if the game process has already died.
    final_listing = subprocess.run([str(loader), "list", str(jdk8)],
                                   capture_output=True, text=True, env=env,
                                   timeout=15, check=True)
    check_target_listing(final_listing.stdout, a.pid)
    # This is not a substitute for a visual game-frame/GL state inspection.
    print("R81_ORIGINAL_RENDER_DIAGNOSTIC_RECEIPTS_PASS")
    print("R81_NOT_GAME_MENU_OR_VISUAL_RENDER_ACCEPTANCE")
    if a.report is not None:
        report = {"scope":"R81 original vanilla render receipts only",
                  "visual_game_acceptance":False,
                  "in_game_menu_verified":False,"stages":records}
        destination = a.report.resolve()
        # Exclusive creation prevents a race between the earlier existence
        # check and a new report supplied by another local process.
        with destination.open("x",encoding="utf-8") as stream:
            stream.write(json.dumps(report,indent=2)+"\n")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(run())
    except (ReceiptError,OSError,subprocess.SubprocessError) as error:
        print("R81_ACCEPTANCE_FAIL_CLOSED: "+str(error),file=sys.stderr)
        sys.exit(3)
