#!/usr/bin/env python3
"""R66: actual JDK8 Attach+retransform+restore against one disposable fixture.

This test never starts, detects, or modifies Minecraft. No game JAR or GL.
"""
import os
import pathlib
import subprocess
import sys

def checked_run(args, env, seconds=25):
    result = subprocess.run(args, capture_output=True, text=True,
                            env=env, timeout=seconds)
    if result.returncode:
        raise AssertionError("Command failed: " + repr(args) + "\n" +
                             result.stdout + result.stderr)
    return result

def run():
    if os.name != "nt":
        raise RuntimeError("Windows x64 Java 8 Attach API test only")
    if len(sys.argv) != 3:
        raise RuntimeError("Expected loader exe and JDK8 home")
    loader=pathlib.Path(sys.argv[1]).resolve()
    home=pathlib.Path(sys.argv[2]).resolve()
    classes=loader.parent/"attach-test-classes"
    manifest=loader.parent/"slinkywinky_r66_test_agent.jar"
    agent_jars=loader.parent/"slinkywinky_instrument.jar"
    java=home/"bin"/"java.exe"
    tools=home/"lib"/"tools.jar"
    for resource in (manifest,agent_jars,tools,java):
        if not resource.is_file():
            raise RuntimeError("Missing required original/isolated test input: " + str(resource))
    env=os.environ.copy()
    env["SLINKYWINKY_R66_DISPOSABLE_FIXTURE"]="1"
    target=subprocess.Popen([
        str(java),"-cp",str(classes)+";"+str(agent_jars),
        "dev.slinkywinky.attach.R65Fixture"],
        stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
        universal_newlines=True,env=env)
    try:
        ready=target.stdout.readline().strip()
        if ready!="R66_DISPOSABLE_RENDER_TARGET_READY":
            raise AssertionError("Unexpected separate JVM target: "+ready)
        caller=checked_run([
            str(java),"-cp",str(classes)+";"+str(tools),
            "dev.slinkywinky.attach.RenderAttachLauncher",
            str(target.pid),str(manifest)],env)
        if "R66_EXTERNAL_JAVA8_ATTACH_PASS" not in caller.stdout:
            raise AssertionError("No separate JVM Attach API receipt")
        output,_=target.communicate(timeout=35)
        markers=[
            "R66_EXACT_FIXTURE_INSTRUMENTATION_PASS",
            "R66_OWNER_FRAME_COUNT_PASS accepted=8",
            "R66_BYTECODE_RESTORE_SHA256_PASS",
            "R66_RENDER_OWNER_CLEANUP_PASS",
            "R66_ORIGINAL_RENDER_BEHAVIOR_PASS frames=16",
            "R66_DISPOSABLE_ATTACH_CLEAN_EXIT"
        ]
        if target.returncode!=0:
            raise AssertionError("Instrumented fixture exited unexpectedly: "+output)
        if any(output.count(marker)!=1 for marker in markers):
            raise AssertionError("Missing/duplicate exact source-restoration evidence: "+
                                 output)
        if "Exception" in output or "Error" in output:
            raise AssertionError("Java exception during R66 Attach sequence: "+output)
        print("R66_ACTUAL_JAVA8_ATTACH_AND_RESTORE_PASS")
        print("R66_READ_ONLY_RENDER_THREAD_PASS")
        print("R66_NO_MINECRAFT_RUNTIME_OR_GL_TOUCHED")
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
