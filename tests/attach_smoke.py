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
    observations = subprocess.run(
        [str(java), "-cp", classpath, "dev.slinkywinky.attach.ObservationSmoke"],
        capture_output=True, text=True, env=env, timeout=15)
    if observations.returncode != 0 or "OBSERVATION_FIELDS_AND_TRANSITIONS_PASS" not in observations.stdout:
        raise AssertionError("Mapped observation regression failed: " +
                             observations.stdout + observations.stderr)
    print("PASS: populated player/target, eye-level, disconnect, reentry and invalid field rejection")
    vanilla = subprocess.run(
        [str(java), "-cp", classpath, "dev.slinkywinky.attach.Vanilla189MappingSmoke"],
        capture_output=True, text=True, env=env, timeout=15)
    if vanilla.returncode != 0 or "VANILLA_189_OBFUSCATED_MAPPING_PASS" not in vanilla.stdout:
        raise AssertionError("Exact vanilla name profile regression: " +
                             vanilla.stdout + vanilla.stderr)
    print("PASS: vanilla 1.8.9 obfuscated tick, world, player and target fields")
    admission = subprocess.run(
        [str(java), "-cp", classpath, "dev.slinkywinky.attach.HookAdmissionSmoke"],
        capture_output=True, text=True, env=env, timeout=15)
    if (admission.returncode != 0 or "HOOK_ADMISSION_RACE_GUARD_PASS" not in admission.stdout or
        "HOOK_DURATION_POLICY_PASS" not in admission.stdout):
        raise AssertionError("Hook overlap admission regression: " +
                             admission.stdout + admission.stderr)
    print("PASS: duplicate hook rejected before shared state mutation; preflight rollback")
    jni_contract = subprocess.run(
        [str(java), "-cp", classpath, "dev.slinkywinky.attach.NativeObservationSmoke",
         str(loader.parent / "slinkywinky_jni.dll")],
        capture_output=True, text=True, env=env, timeout=15)
    if (jni_contract.returncode != 0 or
        "NATIVE_OBSERVATION_CONTRACT_PASS" not in jni_contract.stdout or
        "NATIVE_OBSERVATION_REGISTRY_STRESS_PASS" not in jni_contract.stdout):
        raise AssertionError("Strict native observation ABI regression: " +
                             jni_contract.stdout + jni_contract.stderr)
    print("PASS: native observation JNI contract, malformed frames and wrong-thread rejection")
    # Explicitly opt in ONLY the disposable Java 8 acceptance JVM.
    ipc_env = env.copy()
    ipc_env["SLINKYWINKY_HUD_IPC"] = "1"
    ipc_test = subprocess.run(
        [str(java), "-cp", classpath, "dev.slinkywinky.attach.NativeHudIpcSmoke",
         str(loader.parent / "slinkywinky_jni.dll"),
         str(loader.parent / "slinkywinky_hud_ipc_tests.exe")],
        capture_output=True, text=True, env=ipc_env, timeout=20)
    if (ipc_test.returncode != 0 or
        "JNI_CROSS_PROCESS_HUD_IPC_PASS" not in ipc_test.stdout):
        raise AssertionError("Native JNI -> Windows IPC acceptance failed: " +
                             ipc_test.stdout + ipc_test.stderr)
    print("PASS: opted-in JNI publisher -> separate Win32 reader process")
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
    # The unverified Minecraft tick hook must be explicitly developer-enabled.
    no_gate = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--hook-probe"],
        capture_output=True, text=True, env=env, timeout=15)
    if no_gate.returncode == 0 or "SLINKYWINKY_EXPERIMENTAL_HOOK" not in no_gate.stderr:
        raise AssertionError("Unverified tick hook must be disabled by default")
    hook_env = env.copy()
    hook_env["SLINKYWINKY_EXPERIMENTAL_HOOK"] = "1"
    hook = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--hook-probe"],
        capture_output=True, text=True, env=hook_env, timeout=22)
    if hook.returncode != 0 or "mode=hook-probe" not in hook.stdout:
        raise AssertionError("Read-only tick hook installation failed: " +
                             hook.stdout + hook.stderr)
    # The original 20-tick default must finish before a second admission.
    time.sleep(1.8)
    extended_denied = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--hook-probe=120"],
        capture_output=True, text=True, env=hook_env, timeout=15)
    if extended_denied.returncode == 0 or "SLINKYWINKY_EXTENDED_DIAGNOSTIC" not in extended_denied.stderr:
        raise AssertionError("Long-running diagnostic bypassed explicit gate: " +
                             extended_denied.stdout + extended_denied.stderr)
    malformed = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--hook-probe=999"],
        capture_output=True, text=True, env=hook_env, timeout=15)
    if malformed.returncode == 0:
        raise AssertionError("Arbitrary hook lifespan was accepted")
    extended_env = hook_env.copy()
    extended_env["SLINKYWINKY_EXTENDED_DIAGNOSTIC"] = "1"
    extended = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--hook-probe=120"],
        capture_output=True, text=True, env=extended_env, timeout=22)
    if extended.returncode != 0 or "mode=hook-probe" not in extended.stdout:
        raise AssertionError("Bounded extended native hook refused: " +
                             extended.stdout + extended.stderr)
    # AttachTarget advances one game tick per ~30 ms. Permit 120 observed ticks
    # plus asynchronous class fingerprint restoration before termination.
    time.sleep(5.0)
    target.terminate()
    tail, _ = target.communicate(timeout=5)
    for marker in ("OBSERVATION_RELEASE_PASS", "NATIVE_SESSION_OPEN_PASS", "NATIVE_SESSION_CLOSE_PASS", "OBSERVATION_MAPPING_ADMITTED", "NATIVE_OBSERVATION_PASS world=1 player=1 target=1", "NATIVE_CXX_PREFLIGHT_PASS", "NATIVE_GAME_THREAD_CXX_PASS", "GAME_THREAD_TICK_ENTRY_PASS", "GAME_THREAD_TICK_20_PASS ticks=20", "DIAGNOSTIC_TICK_LIMIT=120",
                   "GAME_THREAD_TICK_120_PASS ticks=120",
                   "CLASS_BYTECODE_RESTORE_PASS", "GAME_THREAD_HOOK_CLEANUP_PASS"):
        if marker not in tail:
            raise AssertionError("Missing tick hook or restoration marker " + marker +
                                 ": " + tail[-1600:])
    if tail.count("CLASS_BYTECODE_RESTORE_PASS") != 2 or tail.count("GAME_THREAD_HOOK_CLEANUP_PASS") != 2:
        raise AssertionError("Both bounded hook sessions must restore exact class bytes twice")
    # R60: inspect only frame counts. Never trust a world/target marker from a
    # rejected native frame, or treat a mock ray hit as attack eligibility.
    import re
    summaries = re.findall(
        r"NATIVE_OBSERVATION_SUMMARY accepted=(\d+) worldPlayer=(\d+) "
        r"rayEntity=(\d+) generations=(\d+)", tail)
    if summaries != [("20", "20", "20", "0"), ("120", "120", "120", "0")]:
        raise AssertionError("Expected two exact validated synthetic world/target summaries: " +
                             repr(summaries))
    if tail.count("NATIVE_WORLD_PLAYER_OBSERVED tick=1") != 2 or (
        tail.count("NATIVE_RAY_ENTITY_OBSERVED tick=1") != 2):
        raise AssertionError("Per-attachment world and ray markers were not reset")
    print("PASS: 20-tick default and opt-in 120-tick native observation; twice-restored bytecode")

    print("PASS: standalone Windows loader -> Java 8 Attach API -> native JVMTI agent")
finally:
    if target.poll() is None:
        target.terminate()
    try:
        target.wait(timeout=5)
    except subprocess.TimeoutExpired:
        target.kill()
        target.wait(timeout=3)
