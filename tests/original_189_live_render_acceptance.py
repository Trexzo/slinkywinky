#!/usr/bin/env python3
"""R81 operator-only original vanilla 1.8.9 render acceptance receipt checker.

Never launches Minecraft, discovers arbitrary processes, modifies source game
files or attaches by default. --execute is an explicit user action. A target
JVM log containing Java-agent System.out (NOT just latest.log by assumption)
must be provided. Only fresh appended bytes count as receipts.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

PREFLIGHT = "R73_LOADED_ORIGINAL_VANILLA_189_RENDERER_PREFLIGHT_PASS"
PREFLIGHT_RELEASED = "R83_ORIGINAL_PREFLIGHT_ADMISSION_RELEASED_PASS"
RESTORED = "R90_RESTORED_SOURCE_PINNED_EXECUTION_PASS"
OVERLAY = "R91_ORIGINAL_RENDER_OVERLAY_DRAW_PASS frames=1"
R92_CALLBACK = "R92_RENDER_CALLBACK_BOUNDED_PASS accepted=72"
R92_STABLE = ("R92_WGL_CONTEXT_STABLE_PASS attempts=72 current=72 "
              "absent=0 invalid=0 changed=0 stable=true frames=64")
R92_OVERLAY = "R92_MULTIFRAME_OVERLAY_DRAW_PASS frames=64"
CALLBACK = "R75_RENDER_CALLBACK_BOUNDED_PASS accepted=8"
RELEASED = "R81_ORIGINAL_RENDER_ADMISSION_RELEASED_PASS"
STABLE = ("R78_READ_ONLY_WGL_CONTEXT_STABLE_PASS "
          "attempts=8 current=8 absent=0 invalid=0 changed=0 stable=true")
# Cap per-stage stdout to prevent unbounded reads or very noisy JVM logs
# being mistaken for a clean, short diagnostic session.
MAX_STAGE_STDOUT_BYTES = 262144
PINNED_VANILLA_189_BYTES = 8461484
PINNED_VANILLA_189_SHA1 = "3870888a6c3d349d3771a3e9d16c9bf5e076b908"
PINNED_VANILLA_189_SHA256 = "14f0d96d1a56fb4f5c3b2233d00699525893fe5ce3dcf181e7de59120595d298"

NEGATIVE = (
    "R75_RENDER_RESTORE_FAIL",
    "R75_RESTORE_UNCERTAIN_NO_REATTACH",
    "R75_RENDER_CALLBACK_UNVERIFIED",
    "R78_READ_ONLY_WGL_CONTEXT_UNVERIFIED",
    "R91_RENDER_OVERLAY_UNVERIFIED",
    "R92_WGL_CONTEXT_UNVERIFIED",
    "R92_MULTIFRAME_OVERLAY_UNVERIFIED",
    "Exception in thread",
    "NoClassDefFoundError",
)
# The external Java8 Attach helper uses a single exact receipt line. It is
# not target-JVM evidence, and the PID/mode must match the stage requested.
LOADER_MODES = {
    "--render-preflight": "render-preflight",
    "--render-callback=8": "render-callback-r75",
    "--render-context=8": "render-context-r78",
    "--render-overlay=8": "render-overlay-r91",
    "--render-overlay=72": "render-overlay-r92",
}
STAGES = {
    "preflight": (("--render-preflight", (PREFLIGHT, PREFLIGHT_RELEASED)),),
    "callback": (("--render-preflight", (PREFLIGHT, PREFLIGHT_RELEASED)),
                 ("--render-callback=8", (RESTORED, CALLBACK, RELEASED))),
    "context": (("--render-preflight", (PREFLIGHT, PREFLIGHT_RELEASED)),
                ("--render-callback=8", (RESTORED, CALLBACK, RELEASED)),
                ("--render-context=8", (RESTORED, CALLBACK, STABLE, RELEASED))),
    "overlay": (("--render-preflight", (PREFLIGHT, PREFLIGHT_RELEASED)),
                ("--render-callback=8", (RESTORED, CALLBACK, RELEASED)),
                ("--render-context=8", (RESTORED, CALLBACK, STABLE, RELEASED)),
                ("--render-overlay=8", (RESTORED, CALLBACK, STABLE, OVERLAY, RELEASED))),
    "multiframe": (("--render-preflight", (PREFLIGHT, PREFLIGHT_RELEASED)),
                   ("--render-callback=8", (RESTORED, CALLBACK, RELEASED)),
                   ("--render-context=8", (RESTORED, CALLBACK, STABLE, RELEASED)),
                   ("--render-overlay=8", (RESTORED, CALLBACK, STABLE, OVERLAY, RELEASED)),
                   ("--render-overlay=72",
                    (RESTORED, R92_CALLBACK, R92_STABLE, R92_OVERLAY, RELEASED))),
}


class ReceiptError(RuntimeError):
    pass


def verify_original_client_jar(path):
    """Read-only local vanilla provenance check; loaded bytecode checked separately."""
    before = path.stat()
    if not path.is_file() or before.st_size != PINNED_VANILLA_189_BYTES:
        raise ReceiptError("Exact original vanilla 1.8.9 client JAR required")
    sha1 = hashlib.sha1()
    sha256 = hashlib.sha256()
    with path.open("rb") as stream:
        opened = os.fstat(stream.fileno())
        if (opened.st_dev, opened.st_ino, opened.st_size) != (
                before.st_dev, before.st_ino, before.st_size):
            raise ReceiptError("Vanilla JAR changed before hashing")
        while True:
            data = stream.read(65536)
            if not data:
                break
            sha1.update(data)
            sha256.update(data)
        after_read = os.fstat(stream.fileno())
    after = path.stat()
    def identity(st):
        return (st.st_dev, st.st_ino, st.st_size, st.st_mtime_ns)
    if (identity(before) != identity(opened) or
            identity(opened) != identity(after_read) or
            identity(after_read) != identity(after)):
        raise ReceiptError("Vanilla JAR changed during verification")
    if (sha1.hexdigest() != PINNED_VANILLA_189_SHA1 or
            sha256.hexdigest() != PINNED_VANILLA_189_SHA256):
        raise ReceiptError("Vanilla 1.8.9 client JAR digest mismatch")
    return identity(after)


def require_same_original_jar(path, original_identity):
    if verify_original_client_jar(path) != original_identity:
        raise ReceiptError("Vanilla JAR replaced between diagnostics")


# R82: validate complete JVM stdout LINES, never permissive substrings.
# In particular, accepted=80 must not satisfy accepted=8; prefixes like
# R90_RESTORED_SOURCE_PINNED_EXECUTION_PASS_FAKE is not a valid receipt.
RECEIPT_LINES = {
    PREFLIGHT: re.compile(r"^" + re.escape(PREFLIGHT) + r"\r?$", re.MULTILINE),
    PREFLIGHT_RELEASED: re.compile(
        r"^" + re.escape(PREFLIGHT_RELEASED) + r"\r?$", re.MULTILINE),
    RESTORED: re.compile(r"^" + re.escape(RESTORED) + r"\r?$", re.MULTILINE),
    CALLBACK: re.compile(
        r"^" + re.escape(CALLBACK) +
        r" owner=[^\r\n]{1,256}#[1-9][0-9]*\r?$", re.MULTILINE),
    RELEASED: re.compile(r"^" + re.escape(RELEASED) + r"\r?$", re.MULTILINE),
    STABLE: re.compile(r"^" + re.escape(STABLE) + r"\r?$", re.MULTILINE),
    OVERLAY: re.compile(r"^" + re.escape(OVERLAY) + r"\r?$", re.MULTILINE),
    R92_CALLBACK: re.compile(
        r"^" + re.escape(R92_CALLBACK) +
        r" owner=[^\r\n]{1,256}#[1-9][0-9]*\r?$", re.MULTILINE),
    R92_STABLE: re.compile(r"^" + re.escape(R92_STABLE) + r"\r?$", re.MULTILINE),
    R92_OVERLAY: re.compile(r"^" + re.escape(R92_OVERLAY) + r"\r?$", re.MULTILINE),
}


def validate_receipt(option, appended):
    """Require one full, fresh target-side line for each expected receipt."""
    expected = dict(STAGES["multiframe"])[option]
    for bad in NEGATIVE:
        if bad in appended:
            raise ReceiptError("Negative target marker present: " + bad)
    # A partial/truncated record that resembles a recognized pass marker
    # is never accepted. Count exact structured lines, including owner fields.
    found = {}
    for marker, pattern in RECEIPT_LINES.items():
        matched = list(pattern.finditer(appended))
        if marker in expected:
            if len(matched) != 1:
                raise ReceiptError("Expected exactly one full target receipt line: " + marker)
            found[marker] = matched[0].start()
        elif matched:
            raise ReceiptError("Unexpected diagnostic stage marker: " + marker)
    # Reject a malformed "pass"-looking line even if a genuine pass line
    # also appears. A target producing conflicting diagnostics is unverified.
    for line in appended.splitlines():
        if any(marker in line for marker in RECEIPT_LINES):
            if not any(
                pattern.fullmatch(line) for pattern in RECEIPT_LINES.values()
            ):
                raise ReceiptError("Malformed or conflicting pass-looking target receipt")
    if option == "--render-preflight":
        ordered = (PREFLIGHT, PREFLIGHT_RELEASED)
    elif option == "--render-overlay=72":
        ordered = (RESTORED, R92_CALLBACK, R92_STABLE, R92_OVERLAY, RELEASED)
    else:
        ordered = ((RESTORED, CALLBACK, STABLE, OVERLAY, RELEASED)
                   if option == "--render-overlay=8" else
                   ((RESTORED, CALLBACK, STABLE, RELEASED)
                    if option == "--render-context=8" else
                    (RESTORED, CALLBACK, RELEASED)))
    positions = [found[marker] for marker in ordered]
    if positions != sorted(positions):
        raise ReceiptError("Renderer restoration / release receipts out of order")
    if "ATTACH_RESULT=SUCCESS" in appended:
        # This belongs to the EXTERNAL loader, not the target JVM.
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
    if size - offset > MAX_STAGE_STDOUT_BYTES:
        raise ReceiptError("Excessive target stdout during one finite diagnostic")
    with path.open("rb") as stream:
        stream.seek(offset)
        raw = stream.read(MAX_STAGE_STDOUT_BYTES + 1)
        if len(raw) > MAX_STAGE_STDOUT_BYTES:
            raise ReceiptError("Target stdout grew beyond bounded evidence window")
        return raw.decode("utf-8", errors="replace")


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


def process_birth_token(pid):
    """Read a Windows PID's kernel creation FILETIME without modifying it.

    The operator can re-use a PID, but a restarted/recycled JVM must never
    inherit earlier diagnostic receipts. No shell, WMI or extra process starts.
    """
    if os.name != "nt":
        raise ReceiptError("Windows PID identity checks require Windows")
    from ctypes import wintypes
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.OpenProcess.argtypes = (wintypes.DWORD, wintypes.BOOL, wintypes.DWORD)
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.GetProcessTimes.argtypes = (
        wintypes.HANDLE, ctypes.POINTER(wintypes.FILETIME),
        ctypes.POINTER(wintypes.FILETIME), ctypes.POINTER(wintypes.FILETIME),
        ctypes.POINTER(wintypes.FILETIME))
    kernel.GetProcessTimes.restype = wintypes.BOOL
    kernel.CloseHandle.argtypes = (wintypes.HANDLE,)
    kernel.CloseHandle.restype = wintypes.BOOL
    handle = kernel.OpenProcess(0x1000, False, pid)  # QUERY_LIMITED_INFORMATION
    if not handle:
        raise ReceiptError("Cannot inspect target PID creation identity")
    try:
        created = wintypes.FILETIME()
        exited = wintypes.FILETIME()
        kernel_time = wintypes.FILETIME()
        user_time = wintypes.FILETIME()
        if not kernel.GetProcessTimes(
                handle, ctypes.byref(created), ctypes.byref(exited),
                ctypes.byref(kernel_time), ctypes.byref(user_time)):
            raise ReceiptError("Cannot read target PID creation time")
        birth = (created.dwHighDateTime << 32) | created.dwLowDateTime
        if not birth:
            raise ReceiptError("Target PID creation identity is invalid")
        return birth
    finally:
        kernel.CloseHandle(handle)


def require_same_process(pid, original_birth):
    current = process_birth_token(pid)
    if current != original_birth:
        raise ReceiptError("Target PID was replaced/restarted during acceptance")


def validate_loader_receipt(option, pid, stdout):
    expected = (
        "ATTACH_RESULT=SUCCESS pid=" + str(pid) +
        " mode=" + LOADER_MODES[option]
    )
    # A different PID or mode (including 1234 when expecting 123), duplicate
    # lines, failure marker or a forged suffix is never a valid Attach receipt.
    lines = stdout.splitlines()
    if len(lines) != 1 or lines[0] != expected:
        raise ReceiptError(
            "External Attach receipt missing, ambiguous or wrong target/mode"
        )


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
    p.add_argument("--original-client-jar", type=Path,
                   help="Optional explicit original Mojang 1.8.9 client JAR path")
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
    if a.original_client_jar is None:
        if not os.environ.get("APPDATA"):
            raise ReceiptError("APPDATA missing; supply --original-client-jar")
        client_path = Path(os.environ["APPDATA"]) / ".minecraft" / (
            "versions/1.8.9/1.8.9.jar")
    else:
        client_path = a.original_client_jar
    client_path = client_path.resolve(strict=True)
    original_client_identity = verify_original_client_jar(client_path)
    print("R89_PINNED_LOCAL_ORIGINAL_189_CLIENT_PASS", flush=True)

    # Do NOT allow a synthetic fixture bypass or an extra gameplay hook gate.
    env = os.environ.copy()
    env.pop("SLINKYWINKY_ATTACH_TEST", None)
    env.pop("SLINKYWINKY_EXPERIMENTAL_HOOK", None)
    env.pop("SLINKYWINKY_RENDER_OVERLAY", None)
    env.pop("SLINKYWINKY_RENDER_MULTIFRAME", None)
    env["SLINKYWINKY_RENDER_PREFLIGHT"] = "1"
    env["SLINKYWINKY_RENDER_CALLBACK"] = "1"
    env["SLINKYWINKY_RENDER_CONTEXT"] = "1"
    if a.mode in ("overlay","multiframe"):
        env["SLINKYWINKY_RENDER_OVERLAY"] = "1"
    if a.mode=="multiframe":
        env["SLINKYWINKY_RENDER_MULTIFRAME"] = "1"

    listed = subprocess.run([str(loader), "list", str(jdk8)],
                            capture_output=True, text=True, env=env,
                            timeout=15, check=True)
    check_target_listing(listed.stdout, a.pid)
    original_birth = process_birth_token(a.pid)
    records = []
    for option, _required in steps:
        # A second check prevents a vanished/replaced PID being attached.
        current = subprocess.run([str(loader), "list", str(jdk8)],
                                 capture_output=True, text=True, env=env,
                                 timeout=15, check=True)
        check_target_listing(current.stdout, a.pid)
        require_same_process(a.pid,original_birth)
        require_same_original_jar(client_path,original_client_identity)
        original_stat = target_log.stat()
        before = original_stat.st_size
        cmd = [str(loader), "attach", str(a.pid), str(jdk8), option]
        result = subprocess.run(cmd,capture_output=True,text=True,
                                env=env,timeout=25)
        if result.returncode != 0:
            raise ReceiptError("External Attach failed for "+option+
                               "; STOP, do not try another diagnostic")
        validate_loader_receipt(option,a.pid,result.stdout)
        require_same_process(a.pid,original_birth)
        receipts = wait_new_receipt(
            target_log,before,original_stat,option,a.timeout_seconds)
        require_same_process(a.pid,original_birth)
        require_same_original_jar(client_path,original_client_identity)
        records.append({"mode":option,"target_receipts":list(receipts)})
        print("R81_FRESH_TARGET_RECEIPTS_PASS mode="+option,flush=True)

    # Receipt success is insufficient if the game process has already died.
    final_listing = subprocess.run([str(loader), "list", str(jdk8)],
                                   capture_output=True, text=True, env=env,
                                   timeout=15, check=True)
    check_target_listing(final_listing.stdout, a.pid)
    require_same_process(a.pid,original_birth)
    require_same_original_jar(client_path,original_client_identity)
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
