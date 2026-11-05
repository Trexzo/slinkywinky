#!/usr/bin/env python3
"""R77 real cross-process Java8 Attach -> generated bfk -> JNI -> owned WGL -> GL11.

The JVM is expendable and starts a Win32 window of its own. No Minecraft/game
process is started, discovered, modified, or targeted by this test.
"""
import os
import pathlib
import subprocess
import sys


def run():
    if os.name != "nt" or len(sys.argv) != 3:
        raise RuntimeError("R77 requires Windows, loader and Java8 JDK")
    loader = pathlib.Path(sys.argv[1]).resolve()
    home = pathlib.Path(sys.argv[2]).resolve()
    classes = loader.parent / "attach-test-classes"
    agent = loader.parent / "slinkywinky_r74_test_agent.jar"
    prod = loader.parent / "slinkywinky_instrument.jar"
    wgl = loader.parent / "slinkywinky_r76_wgl_fixture.dll"
    java = home / "bin" / "java.exe"
    tools = home / "lib" / "tools.jar"
    for path in (agent, prod, wgl, java, tools):
        if not path.is_file():
            raise RuntimeError("Missing R77 isolated test prerequisite: " + str(path))
    env = os.environ.copy()
    env["SLINKYWINKY_R74_DISPOSABLE_FIXTURE"] = "1"
    env["SLINKYWINKY_R76_WGL_FIXTURE"] = "1"
    env["SLINKYWINKY_R77_DISPOSABLE_FIXTURE"] = "1"
    cp = str(classes) + ";" + str(prod)
    target = subprocess.Popen(
        [str(java), "-Djava.library.path=" + str(loader.parent), "-cp", cp,
         "dev.slinkywinky.attach.R77WglRenderTarget"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        universal_newlines=True, env=env)
    try:
        first = target.stdout.readline().strip()
        if first != "R77_DISPOSABLE_WGL_RENDER_TARGET_READY":
            raise AssertionError("Wrong test JVM/native context: " + first)
        launcher = subprocess.run(
            [str(java), "-cp", str(classes) + ";" + str(tools),
             "dev.slinkywinky.attach.R77WglAttachLauncher",
             str(target.pid), str(agent)],
            env=env, capture_output=True, text=True, timeout=30)
        if launcher.returncode or (
                "R77_EXTERNAL_JAVA8_DISPOSABLE_ATTACH_PASS" not in launcher.stdout):
            raise AssertionError("R77 exact test-only Attach denied: " +
                                 launcher.stdout + launcher.stderr)
        tail, _ = target.communicate(timeout=40)
        full = first + "\n" + tail
        markers = (
            "R77_EXACT_GENERATED_WGL_RENDER_HOOK_PASS",
            "R77_ATTACHED_BFK_CALLBACK_TO_NATIVE_GL11_MENU_PASS",
            "R77_JAVA8_RENDER_OWNER_AND_WGL_STATE_RESTORE_PASS",
            "R74_OBFUSCATED_CLASS_RESTORE_SHA256_PASS",
            "R77_JAVA8_ATTACH_BYTECODE_RESTORE_NO_FURTHER_DRAW_PASS",
            "R77_DISPOSABLE_WGL_CONTEXT_CLEAN_EXIT",
        )
        if target.returncode or any(full.count(marker) != 1 for marker in markers):
            raise AssertionError("R77 integrated rendering fixture failed: " + full)
        if "Exception" in full or "Error" in full:
            raise AssertionError("R77 target threw an exception: " + full)
        print("R77_ACTUAL_JAVA8_ATTACH_RENDER_JNI_GL11_PASS")
        print("R77_SYNTHETIC_OBFUSCATED_RENDER_BYTECODE_RESTORE_PASS")
        print("R77_NO_MINECRAFT_RUNTIME_OR_GAME_HOOK")
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
