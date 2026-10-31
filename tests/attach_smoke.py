import os
import pathlib
import subprocess
import sys
import time

loader = pathlib.Path(sys.argv[1]).resolve()
home = pathlib.Path(sys.argv[2]).resolve()
java = home / "bin" / "java.exe"
classes = loader.parent / "attach-test-classes"
env = os.environ.copy()
env["SLINKYWINKY_ATTACH_TEST"] = "1"
target = subprocess.Popen([str(java), "-cp", str(classes),
                           "dev.slinkywinky.attach.AttachTarget"],
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          universal_newlines=True, env=env)
try:
    classpath = str(classes) + ";" + str(loader.parent / "slinkywinky_instrument.jar")
    delegated = subprocess.run(
        [str(java), "-cp", classpath, "dev.slinkywinky.attach.ClassLoaderSmoke"],
        capture_output=True, text=True, env=env, timeout=15)
    if delegated.returncode != 0 or "CLASSLOADER_DELEGATION_PASS" not in delegated.stdout:
        raise AssertionError("Child-first loader admission regression: " +
                             delegated.stdout + delegated.stderr)
    print("PASS: child-first Minecraft-style loader delegates agent callback safely")
    line = target.stdout.readline().strip()
    assert line == "SLINKYWINKY_ATTACH_TARGET_READY", repr(line)
    result = subprocess.run([str(loader), "attach", str(target.pid), str(home)],
                            capture_output=True, text=True, env=env, timeout=22)
    print(result.stdout, result.stderr)
    if result.returncode != 0 or "ATTACH_RESULT=SUCCESS" not in result.stdout:
        raise AssertionError("Native JVM attach failed")
    instrument = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--instrument-probe"],
        capture_output=True, text=True, env=env, timeout=22)
    if instrument.returncode != 0 or "mode=instrument-probe" not in instrument.stdout:
        raise AssertionError("Java 8 Instrumentation live attach probe failed: " +
                             instrument.stdout + instrument.stderr)
    print("PASS: Java 8 Instrumentation agent live read-only retransformation")
    # Production mode must refuse unrelated Java processes even when explicitly given a PID.
    production_env = env.copy()
    production_env.pop("SLINKYWINKY_ATTACH_TEST", None)
    denied = subprocess.run([str(loader), "attach", str(target.pid), str(home)],
                            capture_output=True, text=True, env=production_env, timeout=15)
    if denied.returncode == 0 or "Refusing non-Minecraft target" not in denied.stderr:
        raise AssertionError("Unrelated JVM target unexpectedly admitted")
    print("PASS: non-Minecraft target rejected in production mode")
    hook = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--hook-probe"],
        capture_output=True, text=True, env=env, timeout=22)
    if hook.returncode != 0 or "mode=hook-probe" not in hook.stdout:
        raise AssertionError("Read-only tick hook installation failed: " +
                             hook.stdout + hook.stderr)
    time.sleep(1.8)
    target.terminate()
    tail, _ = target.communicate(timeout=5)
    for marker in ("GAME_THREAD_TICK_ENTRY_PASS", "GAME_THREAD_TICK_20_PASS",
                   "GAME_THREAD_HOOK_CLEANUP_PASS"):
        if marker not in tail:
            raise AssertionError("Missing tick hook or restoration marker " + marker +
                                 ": " + tail[-1600:])
    print("PASS: bounded game-thread tick callback and original-bytecode restoration")

    print("PASS: standalone Windows loader -> Java 8 Attach API -> native JVMTI agent")
finally:
    if target.poll() is None:
        target.terminate()
    try:
        target.wait(timeout=5)
    except subprocess.TimeoutExpired:
        target.kill()
        target.wait(timeout=3)
