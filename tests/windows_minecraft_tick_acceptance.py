#!/usr/bin/env python3
"""Manual-only acceptance for an isolated Windows Minecraft 1.8.9 dev client.

Requires a separately installed/authorized Minecraft+ForgeGradle test project.
No existing .minecraft instance or process is modified; ambiguity fails closed.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import time

p = argparse.ArgumentParser()
p.add_argument("--workspace", type=Path, required=True)
p.add_argument("--jdk8", type=Path, required=True)
p.add_argument("--gradle", type=Path, required=True)
p.add_argument("--forge-project", type=Path, required=True)
a = p.parse_args()
if os.name != "nt":
    sys.exit("This acceptance harness is Windows-only.")
workspace = a.workspace.resolve()
loader = workspace / "build-native-agent" / "Release" / "slinkywinky_loader.exe"
log = workspace / "windows-r38-tick-acceptance.log"
for path in (loader, a.jdk8 / "bin" / "java.exe", a.gradle, a.forge_project):
    if not path.exists():
        sys.exit("Required test component missing: " + str(path))
env = os.environ.copy()
env.pop("SLINKYWINKY_ATTACH_TEST", None)
env["SLINKYWINKY_EXPERIMENTAL_HOOK"] = "1"

def candidates():
    proc = subprocess.run([str(loader), "list", str(a.jdk8)], env=env,
                          capture_output=True, text=True, timeout=12, check=True)
    results = []
    for line in proc.stdout.splitlines():
        match = re.match(r"^([0-9]+)\s+(.+)$", line)
        if match and ("GradleStart" in match.group(2) or
                      "net.minecraft.client.main.Main" in match.group(2)):
            results.append((match.group(1), match.group(2)))
    return results

# Reject an existing development/vanilla JVM to avoid targeting another game.
if candidates():
    sys.exit("Other Minecraft/GradleStart JVM found; refusing ambiguous test target.")
def readlog():
    return log.read_text(encoding="utf-8", errors="replace")

def wait_for(game, condition, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline and game.poll() is None:
        if condition(readlog()):
            return True
        time.sleep(.25)
    return False

markers = ("NATIVE_CXX_PREFLIGHT_PASS", "NATIVE_GAME_THREAD_CXX_PASS",
           "CALLBACK_CLASSLOADER_VALIDATED", "GAME_THREAD_HOOK_INSTALLED",
           "GAME_THREAD_TICK_ENTRY_PASS", "GAME_THREAD_TICK_20_PASS",
           "CLASS_BYTECODE_RESTORE_PASS", "GAME_THREAD_HOOK_CLEANUP_PASS")
with log.open("wb") as output:
    game = subprocess.Popen(
        [str(a.gradle), "--no-daemon", "-p", str(a.forge_project),
         "runClient", "-x", "getAssets"], cwd=str(workspace), env=env,
        stdout=output, stderr=subprocess.STDOUT,
        creationflags=subprocess.CREATE_NEW_PROCESS_GROUP)
    try:
        if not wait_for(game, lambda text:
                        "[SlinkyWinkyPreview] initialized - read-only" in text, 55):
            raise RuntimeError("Isolated Minecraft did not initialize")
        found = candidates()
        if len(found) != 1:
            raise RuntimeError("Ambiguous spawned JVM: " + repr(found))
        pid, descriptor = found[0]
        print("ISOLATED_JVM:", pid, descriptor, flush=True)
        for number in (1, 2):
            attach = subprocess.run([str(loader), "attach", pid, str(a.jdk8),
                                     "--hook-probe"], capture_output=True, text=True,
                                    timeout=18, env=env)
            if attach.returncode or "ATTACH_RESULT=SUCCESS" not in attach.stdout:
                raise RuntimeError("Attachment rejected: " + attach.stdout + attach.stderr)
            if not wait_for(game,
                            lambda text: text.count("GAME_THREAD_HOOK_CLEANUP_PASS") >= number, 15):
                raise RuntimeError("Hook cleanup timeout after attach " + str(number))
            contents = readlog()
            for marker in markers:
                if contents.count(marker) < number:
                    raise RuntimeError("Missing " + marker + " after attach " + str(number))
            if "Game crashed!" in contents or "NoClassDefFoundError" in contents:
                raise RuntimeError("Minecraft crash detected")
            print("PASS: attach", number, "20 ticks, exact cleanup, game alive", flush=True)
        hashes = re.findall(r"CLASS_BYTECODE_RESTORE_PASS sha256=([a-f0-9]{64})", readlog())
        if len(hashes) != 2 or hashes[0] != hashes[1]:
            raise RuntimeError("Original class fingerprint changed across attaches: " + repr(hashes))
        time.sleep(2)
        if game.poll() is not None:
            raise RuntimeError("Minecraft exited after the second cleanup")
        print("R40_LIVE_NATIVE_CXX_REPEATED_HOOK_PASS", hashes[0], flush=True)
    except Exception as err:
        print("R38_LIVE_HOOK_ACCEPTANCE_FAIL", repr(err), flush=True)
        sys.exit(3)
    finally:
        if game.poll() is None:
            subprocess.run(["taskkill", "/PID", str(game.pid), "/T", "/F"],
                           capture_output=True)
            try:
                game.wait(timeout=8)
            except subprocess.TimeoutExpired:
                game.kill()
