#!/usr/bin/env python3
"""R243: deterministic Java 8 Attach failure injection in isolated JVMs only.

Inject faults before a transformer's first registration, first removal,
or first restoration retransform, then require fail-closed admission.
"""
import os
import pathlib
import subprocess
import sys
import zipfile


def read_until(proc, marker, log):
    while True:
        line = proc.stdout.readline()
        if not line:
            raise AssertionError("R243 target ended before " + marker + "\n" +
                                 "".join(log))
        log.append(line)
        if marker in line:
            return


def main():
    if os.name != "nt" or len(sys.argv) != 3:
        raise RuntimeError("Windows Java 8 disposable fixture only")
    loader = pathlib.Path(sys.argv[1]).resolve()
    home = pathlib.Path(sys.argv[2]).resolve()
    directory = loader.parent
    classes = directory / "attach-test-classes"
    agent = directory / "slinkywinky_r241_test_agent.jar"
    production = directory / "slinkywinky_instrument.jar"
    java = home / "bin" / "java.exe"
    tools = home / "lib" / "tools.jar"
    for path in (agent, production, java, tools):
        if not path.is_file():
            raise RuntimeError("R243 missing required fixture: " + str(path))
    with zipfile.ZipFile(production) as jar:
        if any("R243InputAttach" in entry for entry in jar.namelist()):
            raise AssertionError("R243 test code leaked into production JAR")
    with zipfile.ZipFile(agent) as jar:
        if "dev/slinkywinky/instrument/R243InputAttachTarget.class" not in jar.namelist():
            raise AssertionError("R243 isolated JVM target missing")

    for mode in ("install", "remove", "restore"):
        env = os.environ.copy()
        env["SLINKYWINKY_R241_DISPOSABLE_FIXTURE"] = "1"
        env["SLINKYWINKY_R242_DISPOSABLE_NEGATIVE"] = "1"
        env["SLINKYWINKY_R243_DISPOSABLE_NEGATIVE"] = "1"
        env["SLINKYWINKY_R243_FAULT_MODE"] = mode
        target = subprocess.Popen(
            [str(java), "-cp", str(classes) + ";" + str(production),
             "dev.slinkywinky.instrument.R243InputAttachTarget"],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            universal_newlines=True, env=env)
        log = []

        def attach(should_succeed):
            attempt = subprocess.run(
                [str(java), "-cp", str(classes) + ";" + str(tools),
                 "dev.slinkywinky.attach.R242InputAttachLauncher",
                 str(target.pid), str(agent), "valid"],
                capture_output=True, text=True, env=env, timeout=25)
            passed = (attempt.returncode == 0 and
                      attempt.stdout.count("R242_EXTERNAL_JAVA8_ATTACH_ACCEPTED") == 1)
            if passed != should_succeed:
                raise AssertionError("R243 unexpected Attach response mode=" + mode +
                                     "\n" + attempt.stdout + attempt.stderr)

        try:
            read_until(target, "R243_DISPOSABLE_TARGET_READY mode=" + mode, log)
            attach(mode == "restore")
            target.stdin.write("START\n")
            target.stdin.flush()
            if mode == "restore":
                read_until(target, "R243_RESTORE_NEGATIVE_READY", log)
                target.stdin.write("RESTORE\n")
                target.stdin.flush()
                read_until(target, "R243_RESTORE_RECOVERY_BODY_8_PLUS_8_PASS", log)
            else:
                read_until(target,
                           "R243_ABORTED_INSTALL_BODY_UNCHANGED_PASS mode=" + mode,
                           log)
            read_until(target, "R243_POST_FAULT_REATTACH_WINDOW", log)
            attach(False)
            remainder, _ = target.communicate(input="FINISH\n", timeout=30)
            transcript = "".join(log) + remainder
            markers = [
                "R243_REATTACH_AFTER_FAULT_DENIED",
                "R243_DISPOSABLE_" + mode.upper() + "_CLEAN_EXIT",
            ]
            if mode == "install":
                markers += ["R243_TRANSFORMER_INSTALL_REFUSED_BEFORE_MUTATION",
                            "R243_ABORTED_INSTALL_BODY_UNCHANGED_PASS mode=install"]
            elif mode == "remove":
                markers += ["R243_TRANSFORMER_REMOVE_RETRY_AND_RESTORE_SHA256_PASS",
                            "R243_ABORTED_INSTALL_BODY_UNCHANGED_PASS mode=remove"]
            else:
                markers += ["R243_FIRST_RESTORE_CALL_REFUSED",
                            "R243_RESTORE_RETRY_SOURCE_SHA256_PASS",
                            "R243_RESTORE_RECOVERY_BODY_8_PLUS_8_PASS"]
            if target.returncode or any(transcript.count(m) != 1 for m in markers):
                raise AssertionError("R243 missing/duplicate failure evidence mode=" +
                                     mode + "\n" + transcript)
            if mode != "restore" and "R241_REAL_JAVA8_INPUT_RETRANSFORM_PASS" in transcript:
                raise AssertionError("R243 failed install incorrectly marked ready")
            if mode == "restore" and transcript.index(
                "R243_FIRST_RESTORE_CALL_REFUSED") > transcript.index(
                    "R243_RESTORE_RETRY_SOURCE_SHA256_PASS"):
                raise AssertionError("R243 restore was certified before fault handling")
            print("R243_" + mode.upper() +
                  "_CROSS_PROCESS_FAULT_AND_CLEAN_EXIT_PASS")
        finally:
            if target.poll() is None:
                target.terminate()
                try:
                    target.communicate(timeout=5)
                except subprocess.TimeoutExpired:
                    target.kill()
                    target.communicate(timeout=5)
    print("R243_ALL_DISPOSABLE_TRANSFORMER_FAILURE_BOUNDARIES_PASS")
    print("R243_NO_MINECRAFT_PROCESS_OR_REAL_INPUT_INTERCEPTION")


if __name__ == "__main__":
    main()
