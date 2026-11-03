#!/usr/bin/env python3
"""R74 cross-process Attach: synthetic original-name ave/bfk fixtures ONLY.

Never starts or attaches to Minecraft, and does not load graphics libraries.
"""
import os
import pathlib
import subprocess
import sys


def run():
    if os.name != "nt" or len(sys.argv) != 3:
        raise RuntimeError("R74 requires Windows and the loader/JDK8 arguments")
    loader = pathlib.Path(sys.argv[1]).resolve()
    home = pathlib.Path(sys.argv[2]).resolve()
    classes = loader.parent / "attach-test-classes"
    agent = loader.parent / "slinkywinky_r74_test_agent.jar"
    production = loader.parent / "slinkywinky_instrument.jar"
    java = home / "bin" / "java.exe"
    tools = home / "lib" / "tools.jar"
    for file in (java, tools, agent, production):
        if not file.is_file():
            raise RuntimeError("Missing R74 test prerequisite: " + str(file))
    env = os.environ.copy()
    env["SLINKYWINKY_R74_DISPOSABLE_FIXTURE"] = "1"
    cp = str(classes) + ";" + str(production)
    target = subprocess.Popen(
        [str(java), "-cp", cp, "dev.slinkywinky.attach.R74RenderTarget"],
        env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        universal_newlines=True)
    try:
        header = target.stdout.readline().strip()
        if header != "R74_GENERATED_OBFUSCATED_TARGET_READY":
            raise AssertionError("Wrong disposable JVM fixture: " + header)
        attach = subprocess.run(
            [str(java), "-cp", str(classes) + ";" + str(tools),
             "dev.slinkywinky.attach.R74AttachLauncher", str(target.pid),
             str(agent)],
            env=env, capture_output=True, text=True, timeout=25)
        if attach.returncode or "R74_EXTERNAL_JAVA8_ATTACH_PASS" not in attach.stdout:
            raise AssertionError("R74 test-only Attach refused: " +
                                 attach.stdout + attach.stderr)
        tail, _ = target.communicate(timeout=35)
        output = header + "\n" + tail
        markers = (
            "R74_EXACT_OBFUSCATED_FIXTURE_HOOK_PASS",
            "R74_OBFUSCATED_OWNER_CALLBACKS_PASS accepted=8 original=16",
            "R74_WRONG_RENDER_THREAD_FAIL_CLOSED_PASS",
            "R74_OBFUSCATED_CLASS_RESTORE_SHA256_PASS",
            "R74_OBFUSCATED_RENDER_RESTORED_NO_CALLBACK_PASS",
            "R74_DISPOSABLE_PROCESS_CLEAN_EXIT",
        )
        if target.returncode or any(output.count(m) != 1 for m in markers):
            raise AssertionError("R74 exact callback lifecycle failed: " + output)
        if "Exception" in output or "Error" in output:
            raise AssertionError("Unexpected R74 fixture exception: " + output)
        print("R74_GENERATED_OBFUSCATED_CROSS_PROCESS_ATTACH_PASS")
        print("R74_RENDER_OWNER_DENIAL_AND_EXACT_RESTORE_PASS")
        print("R74_NO_MINECRAFT_OR_GRAPHICS_TOUCHED")
    finally:
        if target.poll() is None:
            target.terminate()
            try:
                target.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                target.kill()
                target.communicate(timeout=5)


if __name__ == "__main__":
    run()
