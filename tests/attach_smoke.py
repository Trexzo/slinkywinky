import os
import pathlib
import subprocess
import sys
import time

loader = pathlib.Path(sys.argv[1]).resolve()
home = pathlib.Path(sys.argv[2]).resolve()
java = home / "bin" / "java.exe"
classes = loader.parent / "attach-classes"
env = os.environ.copy()
env["SLINKYWINKY_ATTACH_TEST"] = "1"
target = subprocess.Popen([str(java), "-cp", str(classes),
                           "dev.slinkywinky.attach.AttachTarget"],
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          universal_newlines=True, env=env)
try:
    line = target.stdout.readline().strip()
    assert line == "SLINKYWINKY_ATTACH_TARGET_READY", repr(line)
    result = subprocess.run([str(loader), "attach", str(target.pid), str(home)],
                            capture_output=True, text=True, env=env, timeout=22)
    print(result.stdout, result.stderr)
    if result.returncode != 0 or "ATTACH_RESULT=SUCCESS" not in result.stdout:
        raise AssertionError("Native JVM attach failed")
    print("PASS: standalone Windows loader -> Java 8 Attach API -> native JVMTI agent")
finally:
    target.terminate()
    try:
        target.wait(timeout=5)
    except subprocess.TimeoutExpired:
        target.kill()
        target.wait(timeout=3)
