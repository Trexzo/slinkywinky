#!/usr/bin/env python3
"""R242 negative real Java 8 Attach test against exactly one disposable JVM.

Tests bad options, active and post-restore reattach denial, foreign-thread
observation, and guaranteed source-byte restoration before error reporting.
Never discovers, starts or attaches to Minecraft.
"""
import os
import pathlib
import subprocess
import sys
import zipfile


def require_marker(target, marker, log):
    while True:
        line = target.stdout.readline()
        if not line:
            raise AssertionError("R242 target ended before " + marker + "\n" +
                                 "".join(log))
        log.append(line)
        if marker in line:
            return


def main():
    if os.name != "nt" or len(sys.argv) != 3:
        raise RuntimeError("R242 Windows Java 8 test only")
    loader = pathlib.Path(sys.argv[1]).resolve()
    home = pathlib.Path(sys.argv[2]).resolve()
    directory = loader.parent
    classes = directory / "attach-test-classes"
    test_agent = directory / "slinkywinky_r241_test_agent.jar"
    production = directory / "slinkywinky_instrument.jar"
    java = home / "bin" / "java.exe"
    tools = home / "lib" / "tools.jar"
    for path in (test_agent, production, java, tools):
        if not path.is_file():
            raise RuntimeError("R242 expected build artifact absent: " + str(path))
    with zipfile.ZipFile(production) as jar:
        if any("R242InputAttach" in entry for entry in jar.namelist()):
            raise AssertionError("R242 test-only classes leaked into production JAR")
    with zipfile.ZipFile(test_agent) as jar:
        if "dev/slinkywinky/instrument/R242InputAttachTarget.class" not in jar.namelist():
            raise AssertionError("R242 test target missing from disposable agent JAR")
    env = os.environ.copy()
    env["SLINKYWINKY_R241_DISPOSABLE_FIXTURE"] = "1"
    env["SLINKYWINKY_R242_DISPOSABLE_NEGATIVE"] = "1"
    target = subprocess.Popen(
        [str(java), "-cp", str(classes) + ";" + str(production),
         "dev.slinkywinky.instrument.R242InputAttachTarget"],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        universal_newlines=True, env=env)
    log = []
    def attempt(mode, success):
        trial = subprocess.run(
            [str(java), "-cp", str(classes) + ";" + str(tools),
             "dev.slinkywinky.attach.R242InputAttachLauncher",
             str(target.pid), str(test_agent), mode],
            capture_output=True, text=True, env=env, timeout=25)
        valid = trial.returncode == 0 and trial.stdout.count(
            "R242_EXTERNAL_JAVA8_ATTACH_ACCEPTED") == 1
        if valid != success:
            raise AssertionError(
                "R242 Attach outcome mismatch mode=" + mode +
                " expected_success=" + str(success) + "\n" +
                trial.stdout + trial.stderr)
    try:
        require_marker(target, "R242_DISPOSABLE_INPUT_TARGET_READY", log)
        attempt("bad-options", False)
        attempt("valid", True)
        require_marker(
            target, "R242_FOREIGN_ENTRY_OBSERVED_AND_DUPLICATE_WINDOW_READY", log)
        attempt("valid", False)
        target.stdin.write("RESTORE\n")
        target.stdin.flush()
        require_marker(target, "R242_POST_RESTORE_REATTACH_WINDOW_READY", log)
        attempt("valid", False)
        remainder, _ = target.communicate(input="FINISH\n", timeout=30)
        transcript = "".join(log) + remainder
        markers = (
            "R242_INVALID_AGENT_OPTIONS_REFUSED",
            "R241_REAL_JAVA8_INPUT_RETRANSFORM_PASS",
            "R242_FOREIGN_ENTRY_OBSERVED_AND_DUPLICATE_WINDOW_READY",
            "R241_REAL_JAVA8_INPUT_BYTECODE_RESTORE_SHA256_PASS",
            "R242_RESTORED_BEFORE_INVALID_COUNT_REJECTION entries=8 foreign=1",
            "R242_FOREIGN_THREAD_COUNT_REJECTED_AFTER_SAFE_RESTORE",
            "R242_POST_RESTORE_REATTACH_WINDOW_READY",
            "R242_ORIGINAL_INPUT_BODY_9_PLUS_8_PASS calls=17",
            "R242_DISPOSABLE_FAILURE_CASE_CLEAN_EXIT",
        )
        if target.returncode or any(transcript.count(m) != 1 for m in markers):
            raise AssertionError("R242 missing/duplicate receipts:\n" + transcript)
        if transcript.count("R242_DUPLICATE_ATTACH_REFUSED") != 2:
            raise AssertionError("R242 active/post-restore reattach was not denied:\n" +
                                 transcript)
        order = [
            "R241_REAL_JAVA8_INPUT_BYTECODE_RESTORE_SHA256_PASS",
            "R242_RESTORED_BEFORE_INVALID_COUNT_REJECTION",
            "R242_FOREIGN_THREAD_COUNT_REJECTED_AFTER_SAFE_RESTORE",
        ]
        if not (transcript.index(order[0]) < transcript.index(order[1]) <
                transcript.index(order[2])):
            raise AssertionError("R242 failed restoration-before-refusal ordering")
        print("R242_REAL_ATTACH_BAD_OPTION_AND_DUPLICATE_DENIAL_PASS")
        print("R242_FOREIGN_THREAD_FAIL_CLOSED_AFTER_SOURCE_RESTORE_PASS")
        print("R242_NO_MINECRAFT_INPUT_INTERCEPTION_OR_INPUT_DELIVERY")
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
