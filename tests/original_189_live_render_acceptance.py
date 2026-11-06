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
from windows_r95_scoped_insert import send_isolated_insert

PREFLIGHT = "R73_LOADED_ORIGINAL_VANILLA_189_RENDERER_PREFLIGHT_PASS"
PREFLIGHT_RELEASED = "R83_ORIGINAL_PREFLIGHT_ADMISSION_RELEASED_PASS"
RESTORED = "R90_RESTORED_SOURCE_PINNED_EXECUTION_PASS"
OVERLAY = "R91_ORIGINAL_RENDER_OVERLAY_DRAW_PASS frames=1"
R92_CALLBACK = "R92_RENDER_CALLBACK_BOUNDED_PASS accepted=72"
R92_STABLE = ("R92_WGL_CONTEXT_STABLE_PASS attempts=72 current=72 "
              "absent=0 invalid=0 changed=0 stable=true frames=64")
R92_OVERLAY = "R92_MULTIFRAME_OVERLAY_DRAW_PASS frames=64"
R99_CALLBACK = "R99_RENDER_CALLBACK_BOUNDED_PASS accepted=72"
R99_STABLE = ("R99_WGL_CONTEXT_STABLE_PASS attempts=72 current=72 "
              "absent=0 invalid=0 changed=0 stable=true "
              "frames=64 retained=true actions=4 tab=1 focus=17")
R99_RETAINED = "R99_RETAINED_MENU_DRAW_PASS frames=64 actions=4 tab=1 focus=17"
R100_CALLBACK = "R100_RENDER_CALLBACK_BOUNDED_PASS"
R100_STABLE = "R100_WGL_CONTEXT_STABLE_PASS"
R100_OPERATOR = "R100_OPERATOR_MENU_CONTROLLED_PASS actions=4 tab=1 focus=17 preview=1 stop=operator"
R103_CALLBACK = "R101_RENDER_CALLBACK_BOUNDED_PASS accepted=8"
R103_STABLE = "R101_WGL_CONTEXT_STABLE_PASS"
R103_WINDOW = "R101_ORIGINAL_GAME_WINDOW_IDENTIFIED_PASS"
R93_CALLBACK = "R93_RENDER_CALLBACK_BOUNDED_PASS accepted=72"
R93_STABLE = ("R93_WGL_CONTEXT_STABLE_PASS attempts=72 current=72 "
              "absent=0 invalid=0 changed=0 stable=true shown=48 hidden=16")
R93_VISIBILITY = "R93_MENU_SHOW_HIDE_SHOW_PASS shown=48 hidden=16"
R94_CALLBACK = "R94_RENDER_CALLBACK_BOUNDED_PASS"
R94_STABLE = "R94_WGL_CONTEXT_STABLE_PASS"
R94_VISIBILITY = "R94_MENU_OPERATOR_CONTROLLED_PASS"
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
    "R93_WGL_CONTEXT_UNVERIFIED",
    "R93_MENU_VISIBILITY_UNVERIFIED",
    "R94_WGL_CONTEXT_UNVERIFIED",
    "R94_OPERATOR_VISIBILITY_UNVERIFIED",
    "R99_WGL_CONTEXT_UNVERIFIED",
    "R99_RETAINED_MENU_UNVERIFIED",
    "R100_WGL_CONTEXT_UNVERIFIED",
    "R100_OPERATOR_MENU_UNVERIFIED",
    "R101_WGL_CONTEXT_UNVERIFIED",
    "R101_GAME_WINDOW_IDENTITY_UNVERIFIED",
    "Exception in thread",
    "NoClassDefFoundError",
)
# The external Java8 Attach helper uses a single exact receipt line. It is
# not target-JVM evidence, and the PID/mode must match the stage requested.
LOADER_MODES = {
    "--render-preflight": "render-preflight",
    "--render-callback=8": "render-callback-r75",
    "--render-context=8": "render-context-r78",
    "--render-window-audit=8": "render-window-audit-r103",
    "--render-overlay=8": "render-overlay-r91",
    "--render-overlay=72": "render-overlay-r92",
    "--render-visibility=72": "render-visibility-r93",
    "--render-visibility=manual": "render-manual-r94",
    "--render-retained=72": "render-retained-r99",
    "--render-menu=operator": "render-menu-r100",
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
    "visibility": (("--render-preflight", (PREFLIGHT, PREFLIGHT_RELEASED)),
                   ("--render-callback=8", (RESTORED, CALLBACK, RELEASED)),
                   ("--render-context=8", (RESTORED, CALLBACK, STABLE, RELEASED)),
                   ("--render-overlay=8", (RESTORED, CALLBACK, STABLE, OVERLAY, RELEASED)),
                   ("--render-overlay=72",
                    (RESTORED, R92_CALLBACK, R92_STABLE, R92_OVERLAY, RELEASED)),
                   ("--render-visibility=72",
                    (RESTORED, R93_CALLBACK, R93_STABLE, R93_VISIBILITY, RELEASED))),
    "retained": (("--render-preflight", (PREFLIGHT, PREFLIGHT_RELEASED)),
                 ("--render-callback=8", (RESTORED, CALLBACK, RELEASED)),
                 ("--render-context=8", (RESTORED, CALLBACK, STABLE, RELEASED)),
                 ("--render-overlay=8", (RESTORED, CALLBACK, STABLE, OVERLAY, RELEASED)),
                 ("--render-overlay=72",
                  (RESTORED, R92_CALLBACK, R92_STABLE, R92_OVERLAY, RELEASED)),
                 ("--render-retained=72",
                  (RESTORED, R99_CALLBACK, R99_STABLE, R99_RETAINED, RELEASED))),
    "manual": (("--render-preflight", (PREFLIGHT, PREFLIGHT_RELEASED)),
               ("--render-callback=8", (RESTORED, CALLBACK, RELEASED)),
               ("--render-context=8", (RESTORED, CALLBACK, STABLE, RELEASED)),
               ("--render-overlay=8", (RESTORED, CALLBACK, STABLE, OVERLAY, RELEASED)),
               ("--render-overlay=72",
                (RESTORED, R92_CALLBACK, R92_STABLE, R92_OVERLAY, RELEASED)),
               ("--render-visibility=72",
                (RESTORED, R93_CALLBACK, R93_STABLE, R93_VISIBILITY, RELEASED)),
               ("--render-visibility=manual",
                (RESTORED, R94_CALLBACK, R94_STABLE, R94_VISIBILITY, RELEASED))),
}

# R103 independently links the source-pinned native WGL HWND audit to
# the external exact-PID/birth/unique-window R102 observation.
STAGES["window"] = STAGES["context"] + (
    ("--render-window-audit=8",
     (RESTORED,R103_CALLBACK,R103_STABLE,R103_WINDOW,RELEASED)),
)

# R100 includes all six R99 original class/render/GL admissions before
# the new independent externally-commanded operator stage.
STAGES["operator"] = STAGES["retained"] + (
    ("--render-menu=operator",
     (RESTORED,R100_CALLBACK,R100_STABLE,R100_OPERATOR,RELEASED)),
)

# R95 uses the same seven original-JVM admissions as R94, but the final
# toggle inputs must be OS-injected Insert on the exact sandbox window.
STAGES["key"] = STAGES["manual"]


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
    R99_CALLBACK: re.compile(
        r"^" + re.escape(R99_CALLBACK) +
        r" owner=[^\r\n]{1,256}#[1-9][0-9]*\r?$", re.MULTILINE),
    R99_STABLE: re.compile(r"^" + re.escape(R99_STABLE) + r"\r?$", re.MULTILINE),
    R99_RETAINED: re.compile(r"^" + re.escape(R99_RETAINED) + r"\r?$", re.MULTILINE),
    R100_CALLBACK: re.compile(
        r"^R100_RENDER_CALLBACK_BOUNDED_PASS accepted=([0-9]{1,4}) "
        r"owner=[^\r\n]{1,256}#[1-9][0-9]*\r?$",re.MULTILINE),
    R100_STABLE: re.compile(
        r"^R100_WGL_CONTEXT_STABLE_PASS attempts=([0-9]{1,4}) "
        r"current=([0-9]{1,4}) absent=0 invalid=0 changed=0 stable=true "
        r"shown=([0-9]{1,4}) actions=4 refused=0 tab=1 "
        r"focus=17 preview=1 stop=operator\r?$",re.MULTILINE),
    R100_OPERATOR: re.compile(
        r"^" + re.escape(R100_OPERATOR) + r"\r?$",re.MULTILINE),
    R103_CALLBACK: re.compile(
        r"^" + re.escape(R103_CALLBACK) +
        r" owner=[^\r\n]{1,256}#[1-9][0-9]*\r?$",re.MULTILINE),
    R103_STABLE: re.compile(
        r"^R101_WGL_CONTEXT_STABLE_PASS attempts=8 current=8 "
        r"absent=0 invalid=0 changed=0 stable=true "
        r"window=exact-original owner=render-thread "
        r"foreground=([0-8]) background=([0-8]) refused=0 "
        r"width=([0-9]{3,4}) height=([0-9]{3,4})\r?$",re.MULTILINE),
    R103_WINDOW: re.compile(r"^" + re.escape(R103_WINDOW) + r"\r?$",re.MULTILINE),
    R93_CALLBACK: re.compile(
        r"^" + re.escape(R93_CALLBACK) +
        r" owner=[^\r\n]{1,256}#[1-9][0-9]*\r?$", re.MULTILINE),
    R93_STABLE: re.compile(r"^" + re.escape(R93_STABLE) + r"\r?$", re.MULTILINE),
    R93_VISIBILITY: re.compile(r"^" + re.escape(R93_VISIBILITY) + r"\r?$", re.MULTILINE),
    R94_CALLBACK: re.compile(
        r"^R94_RENDER_CALLBACK_BOUNDED_PASS accepted=([0-9]{1,4}) "
        r"owner=[^\r\n]{1,256}#[1-9][0-9]*\r?$", re.MULTILINE),
    R94_STABLE: re.compile(
        r"^R94_WGL_CONTEXT_STABLE_PASS attempts=([0-9]{1,4}) current=([0-9]{1,4}) "
        r"absent=0 invalid=0 changed=0 stable=true "
        r"shown=([0-9]{1,4}) hidden=([0-9]{1,4}) toggles=([0-9]{1,4}) "
        r"stop=operator\r?$", re.MULTILINE),
    R94_VISIBILITY: re.compile(
        r"^R94_MENU_OPERATOR_CONTROLLED_PASS frames=([0-9]{1,4}) "
        r"toggles=([0-9]{1,4}) shown=([0-9]{1,4}) "
        r"hidden=([0-9]{1,4}) stop=operator\r?$", re.MULTILINE),
}


def validate_receipt(option, appended):
    """Require one full, fresh target-side line for each expected receipt."""
    expected = dict(STAGES["operator"] + STAGES["manual"] + STAGES["window"])[option]
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
    if option=="--render-window-audit=8":
        window_counts=RECEIPT_LINES[R103_STABLE].search(appended)
        if not window_counts:
            raise ReceiptError("R103 original renderer/window samples missing")
        fg,bg,w,h=map(int,window_counts.groups())
        if fg+bg!=8 or not (320<=w<=8192 and 240<=h<=8192):
            raise ReceiptError("R103 original renderer foreground/background/size inconsistent")
    if option=="--render-visibility=manual":
        m0=RECEIPT_LINES[R94_CALLBACK].search(appended)
        m1=RECEIPT_LINES[R94_STABLE].search(appended)
        m2=RECEIPT_LINES[R94_VISIBILITY].search(appended)
        counts=tuple(map(int,m2.groups())) if m2 else ()
        if not (m0 and m1 and m2 and len(counts)==4):
            raise ReceiptError("Missing R94 numeric target receipts")
        frames,toggles,shown,hidden=counts
        if not (9 <= frames <= 2048 and 2 <= toggles <= frames-8 and
                shown>0 and hidden>0 and shown+hidden==frames-8 and
                int(m0.group(1))==frames and
                tuple(map(int,m1.groups()))==
                (frames,frames,shown,hidden,toggles)):
            raise ReceiptError("R94 counter relationship or stop evidence invalid")
    if option=="--render-menu=operator":
        cmd=RECEIPT_LINES[R100_CALLBACK].search(appended)
        status=RECEIPT_LINES[R100_STABLE].search(appended)
        if not cmd or not status:
            raise ReceiptError("Missing R100 actual original render-thread counters")
        frames=int(cmd.group(1))
        attempts,current,shown=tuple(map(int,status.groups()))
        if not (9<=frames<=2048 and frames==attempts==current and
                shown==frames-8):
            raise ReceiptError("R100 operator callback/frame provenance mismatch")
    if option == "--render-preflight":
        ordered = (PREFLIGHT, PREFLIGHT_RELEASED)
    elif option == "--render-window-audit=8":
        ordered = (RESTORED,R103_CALLBACK,R103_STABLE,R103_WINDOW,RELEASED)
    elif option == "--render-menu=operator":
        ordered = (RESTORED,R100_CALLBACK,R100_STABLE,R100_OPERATOR,RELEASED)
    elif option == "--render-retained=72":
        ordered = (RESTORED, R99_CALLBACK, R99_STABLE, R99_RETAINED, RELEASED)
    elif option == "--render-visibility=manual":
        ordered = (RESTORED, R94_CALLBACK, R94_STABLE, R94_VISIBILITY, RELEASED)
    elif option == "--render-visibility=72":
        ordered = (RESTORED, R93_CALLBACK, R93_STABLE, R93_VISIBILITY, RELEASED)
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
    env.pop("SLINKYWINKY_RENDER_VISIBILITY", None)
    env.pop("SLINKYWINKY_RENDER_MANUAL", None)
    env.pop("SLINKYWINKY_RENDER_RETAINED", None)
    env.pop("SLINKYWINKY_RENDER_OPERATOR", None)
    env.pop("SLINKYWINKY_RENDER_WINDOW", None)
    env["SLINKYWINKY_RENDER_PREFLIGHT"] = "1"
    env["SLINKYWINKY_RENDER_CALLBACK"] = "1"
    env["SLINKYWINKY_RENDER_CONTEXT"] = "1"
    if a.mode in ("overlay","multiframe","visibility","manual","key","retained","operator"):
        env["SLINKYWINKY_RENDER_OVERLAY"] = "1"
    if a.mode in ("multiframe","visibility","manual","key","retained","operator"):
        # R93 must pass the earlier, separately consent-gated R92 stage.
        env["SLINKYWINKY_RENDER_MULTIFRAME"] = "1"
    if a.mode in ("visibility","manual","key"):
        env["SLINKYWINKY_RENDER_VISIBILITY"] = "1"
    if a.mode in ("manual","key"):
        env["SLINKYWINKY_RENDER_MANUAL"] = "1"
    if a.mode in ("retained","operator"):
        env["SLINKYWINKY_RENDER_RETAINED"] = "1"
    if a.mode=="operator":
        env["SLINKYWINKY_RENDER_OPERATOR"] = "1"
    if a.mode=="window":
        env["SLINKYWINKY_RENDER_WINDOW"] = "1"

    listed = subprocess.run([str(loader), "list", str(jdk8)],
                            capture_output=True, text=True, env=env,
                            timeout=15, check=True)
    check_target_listing(listed.stdout, a.pid)
    original_birth = process_birth_token(a.pid)
    records = []
    window_crosscheck = None
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
        external_before = None
        if option == "--render-window-audit=8":
            from windows_r102_window_identity_evidence import (
                observe_owned_game,WindowAuditError)
            try:
                external_before=observe_owned_game(a.pid,original_birth)
            except WindowAuditError as problem:
                raise ReceiptError("R103 external pre-render original HWND refused: "+str(problem))
            require_same_process(a.pid,original_birth)
        cmd = [str(loader), "attach", str(a.pid), str(jdk8), option]
        result = subprocess.run(cmd,capture_output=True,text=True,
                                env=env,timeout=25)
        if result.returncode != 0:
            # R93: print only the Java8 helper's declared error category.
            # Never echo arbitrary target stdout/stderr or filesystem paths.
            failed = next((line for line in result.stderr.splitlines()
                           if line.startswith("ATTACH_RESULT=FAIL ")), "")
            category = re.match(
                r"^ATTACH_RESULT=FAIL ([A-Za-z][A-Za-z0-9_$]{0,60})", failed)
            detail = (" helper="+category.group(1)) if category else ""
            raise ReceiptError("External Attach failed for "+option+detail+
                               "; STOP, do not try another diagnostic")
        validate_loader_receipt(option,a.pid,result.stdout)
        require_same_process(a.pid,original_birth)
        if option == "--render-menu=operator":
            # Five distinct commands originate in this supervised owner-PID
            # helper process, NEVER from the target JVM's scripted R99 path.
            # Every response is acknowledged by the source-admitted original
            # WGL render owner; no SendInput or Minecraft key/mouse hooks.
            controls=("--menu-next-row","--menu-expand",
                      "--menu-toggle-preview","--menu-next-tab",
                      "--menu-stop")
            for control in controls:
                require_same_process(a.pid,original_birth)
                command=[str(loader),"attach",str(a.pid),str(jdk8),control]
                reply=subprocess.run(command,capture_output=True,text=True,
                                     timeout=8,env=env)
                expected="ATTACH_RESULT=SUCCESS pid="+str(a.pid)+" mode="+control[2:]
                if reply.returncode or reply.stdout.strip()!=expected:
                    # Best-effort cooperative stop, only for the same
                    # kernel-birth original JVM; don't claim restoration
                    # unless the target subsequently emits its full proof.
                    try:
                        require_same_process(a.pid,original_birth)
                        subprocess.run(
                            [str(loader),"attach",str(a.pid),str(jdk8),
                             "--menu-stop"],
                            capture_output=True,text=True,env=env,timeout=4)
                    except Exception:
                        pass
                    raise ReceiptError("R100 operator command "+control+
                                       " not acknowledged by original render owner")
                print("R100_ORIGINAL_MENU_OPERATOR_ACK_PASS command="+control,flush=True)
        if option == "--render-visibility=manual":
            # All controls are explicit PID-scoped events, not Java Attach.
            # The supervised offline game owns this process and can safely
            # simulate an operator toggling a menu twice, then stopping it.
            if a.mode=="key":
                # The keyboard helper refuses a foreign/minimized window and
                # records ONLY synthetic OS SendInput: never physical hardware.
                try:
                    sent=send_isolated_insert(a.pid,target_log.parent)
                    if sent!=2:
                        raise RuntimeError("R95 synthetic key count incomplete")
                except Exception:
                    # Best effort STOP of only the exact same original JVM.
                    # Never claim restored-source acceptance merely because
                    # the stop event was posted; the owning launcher still
                    # terminates only its sandbox child on refusal.
                    try:
                        require_same_process(a.pid,original_birth)
                        subprocess.run(
                            [str(loader),"attach",str(a.pid),str(jdk8),
                             "--stop-visibility"],
                            capture_output=True,text=True,env=env,timeout=8)
                    except Exception:
                        pass
                    raise ReceiptError(
                        "R95 owner-foreground SendInput admission refused; "
                        "requested cooperative stop; no key PASS") from None
                controls=(("--stop-visibility",0.35),)
            else:
                controls=(("--toggle-visibility",0.30),
                          ("--toggle-visibility",0.40),
                          ("--toggle-visibility",0.30),
                          ("--stop-visibility",0.40))
            for control,delay in controls:
                time.sleep(delay)
                require_same_process(a.pid,original_birth)
                command=[str(loader),"attach",str(a.pid),str(jdk8),control]
                reply=subprocess.run(command,capture_output=True,text=True,
                                     timeout=8,env=env)
                expected=("ATTACH_RESULT=SUCCESS pid="+str(a.pid)+
                          " mode="+control[2:])
                if reply.returncode or reply.stdout.strip()!=expected:
                    # Never echo arbitrary process output. Categorize only
                    # the target-gate lookup failure we explicitly recognize.
                    category=(" gate_unavailable" if
                              "no active R94 visibility gate" in reply.stderr
                              else (" mismatched_receipt" if reply.returncode==0
                                    else " control_refused"))
                    raise ReceiptError("R94 PID-scoped operator "+control+
                                       category+"; STOP and preserve fail-closed restore")
        receipts = wait_new_receipt(
            target_log,before,original_stat,option,a.timeout_seconds)
        if option == "--render-window-audit=8":
            # R102 external HWND evidence brackets the source-verified R101
            # original GL render-thread HDC audit on the SAME birth-pinned JVM.
            try:
                external_after=observe_owned_game(a.pid,original_birth)
            except WindowAuditError as problem:
                raise ReceiptError("R103 external post-render original HWND refused: "+str(problem))
            require_same_process(a.pid,original_birth)
            appended=read_appended(target_log,before,original_stat)
            native=RECEIPT_LINES[R103_STABLE].search(appended)
            if native is None or external_before is None:
                raise ReceiptError("R103 native renderer/window evidence absent")
            fg,bg,width,height=tuple(map(int,native.groups()))
            before_geometry=external_before["window"]
            after_geometry=external_after["window"]
            if (before_geometry["pid"]!=a.pid or after_geometry["pid"]!=a.pid or
                (before_geometry["width"],before_geometry["height"])!=(width,height) or
                (after_geometry["width"],after_geometry["height"])!=(width,height) or
                before_geometry["identity"]!=after_geometry["identity"] or
                (fg+bg)!=8 or external_before["refused_samples"] or
                external_after["refused_samples"]):
                raise ReceiptError("R103 source-pinned native WGL viewport and external unique HWND disagree")
            window_crosscheck={
                "original_pid_birth_verified":True,
                "external_pre":external_before,
                "native_render_window":{
                    "samples":8,"foreground":fg,"background":bg,
                    "viewport_width":width,"viewport_height":height,
                    "original_gl_hdc_window_identity_pass":True,
                },
                "external_post":external_after,
                "same_original_window_size_and_unique_identity":True,
                "foreground_or_input_capture_certified":False,
                "os_input_sent":False,
            }
            print("R103_ORIGINAL_RENDER_HDC_EXTERNAL_WINDOW_RECONCILED_PASS",flush=True)
        if a.mode=="key" and option=="--render-visibility=manual":
            # This is an extra source-specific audit, not a replacement for
            # exact original-JVM class restoration, WGL and gate release.
            appended=read_appended(target_log,before,original_stat)
            match=re.findall(
                r"(?m)^R95_INPUT_SOURCE_AUDIT keys=([0-9]{1,4}) "
                r"events=([0-9]{1,4})\r?$",appended)
            if match!=[("2","0")] or appended.count("R95_INPUT_SOURCE_AUDIT")!=1:
                raise ReceiptError("R95 native Insert vs PID event provenance unverified")
            final=re.search(
                r"(?m)^R94_MENU_OPERATOR_CONTROLLED_PASS frames=([0-9]{1,4}) "
                r"toggles=([0-9]{1,4}) shown=([0-9]{1,4}) "
                r"hidden=([0-9]{1,4}) stop=operator\r?$",appended)
            if final is None or int(final.group(2))!=2:
                raise ReceiptError("R95 Insert must be the ONLY visibility toggle source")
            print("R95_ORIGINAL_FOREGROUND_SYNTHETIC_INSERT_TWICE_PASS",flush=True)
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
        if a.mode=="window":
            if window_crosscheck is None:
                raise ReceiptError("R103 missing independent native/external window reconciliation")
            report["window_crosscheck"]=window_crosscheck
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
