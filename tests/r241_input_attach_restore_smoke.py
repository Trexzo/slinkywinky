#!/usr/bin/env python3
"""R241 real Java 8 Attach/weave/restore against a disposable input fixture.

Never selects, starts, attaches to or searches for a Minecraft JVM.
"""
import os
import pathlib
import subprocess
import sys
import zipfile


def main():
    if os.name != "nt" or len(sys.argv) != 3:
        raise RuntimeError("Windows x64 isolated Java 8 fixture arguments required")
    loader = pathlib.Path(sys.argv[1]).resolve()
    home = pathlib.Path(sys.argv[2]).resolve()
    directory = loader.parent
    classes = directory / "attach-test-classes"
    test_agent = directory / "slinkywinky_r241_test_agent.jar"
    production = directory / "slinkywinky_instrument.jar"
    java = home / "bin" / "java.exe"
    tools = home / "lib" / "tools.jar"
    for item in (test_agent, production, java, tools):
        if not item.is_file():
            raise RuntimeError("R241 missing required fixture: " + str(item))
    with zipfile.ZipFile(production) as jar:
        if any("R241InputAttach" in name for name in jar.namelist()):
            raise AssertionError("R241 disposable test code leaked into production agent")
    with zipfile.ZipFile(test_agent) as jar:
        if "dev/slinkywinky/instrument/R241InputAttachAgent.class" not in jar.namelist():
            raise AssertionError("R241 test agent entry class absent")

    env = os.environ.copy()
    env["SLINKYWINKY_R241_DISPOSABLE_FIXTURE"] = "1"
    target = subprocess.Popen(
        [str(java), "-cp", str(classes) + ";" + str(production),
         "dev.slinkywinky.instrument.R241InputAttachTarget"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        universal_newlines=True, env=env)
    try:
        ready = target.stdout.readline().strip()
        if ready != "R241_DISPOSABLE_INPUT_TARGET_READY":
            raise AssertionError("R241 wrong disposable JVM: " + ready)
        attach = subprocess.run(
            [str(java), "-cp", str(classes) + ";" + str(tools),
             "dev.slinkywinky.attach.R241InputAttachLauncher",
             str(target.pid), str(test_agent)],
            capture_output=True, text=True, env=env, timeout=25)
        if attach.returncode or attach.stdout.count(
                "R241_EXTERNAL_JAVA8_ATTACH_PASS") != 1:
            raise AssertionError("R241 cross-process Attach failed:\n" +
                                 attach.stdout + attach.stderr)
        output, _ = target.communicate(timeout=35)
        markers = (
            "R241_REAL_JAVA8_INPUT_RETRANSFORM_PASS",
            "R241_OWNER_INPUT_ENTRY_COUNT_PASS count=8",
            "R241_REAL_JAVA8_INPUT_BYTECODE_RESTORE_SHA256_PASS",
            "R241_ORIGINAL_INPUT_BODY_8_PLUS_8_PASS calls=16",
            "R241_DISPOSABLE_INPUT_ATTACH_CLEAN_EXIT",
        )
        if target.returncode or any(output.count(m) != 1 for m in markers):
            raise AssertionError("R241 missing/duplicate restoration receipts:\n" + output)
        if "Exception" in output or "Error" in output:
            raise AssertionError("R241 unexpected fixture exception:\n" + output)
        print("R241_CROSS_PROCESS_JAVA8_INPUT_ATTACH_RESTORE_PASS")
        print("R241_NO_MINECRAFT_INPUT_CONSUMERS_MODIFIED")
    finally:
        if target.poll() is None:
            target.terminate()
            try:
                target.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                target.kill()
                target.communicate(timeout=5)


if __name__ == "__main__":
    main()
