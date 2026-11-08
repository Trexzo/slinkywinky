import os
import faulthandler
import pathlib
import subprocess
import sys
import time
import zipfile

faulthandler.enable()
# Diagnostic only: if a subprocess or pipe deadlocks on Windows, emit the
# Python stack to CTest's retained log instead of silently timing out.
faulthandler.dump_traceback_later(25, repeat=True)
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
    instrument_jar = loader.parent / "slinkywinky_instrument.jar"
    # R119: test-only ASM/input-audit and mock-LWJGL code must not be
    # bundled into the runtime Java 8 instrument jar.
    with zipfile.ZipFile(instrument_jar, "r") as agent:
        entries = set(agent.namelist())
        required = {
            "dev/slinkywinky/instrument/KeyboardQueueOwner.class",
            "dev/slinkywinky/instrument/KeyboardPageRelay.class",
            "dev/slinkywinky/instrument/MouseQueueOwner.class",
        }
        forbidden = {
            "org/lwjgl/input/Keyboard.class",
            "org/lwjgl/input/Mouse.class",
            "dev/slinkywinky/attach/MouseQueueOwnerSmoke.class",
            "dev/slinkywinky/attach/R161InputCoverageLedger.class",
            "dev/slinkywinky/attach/R161PairedInputRollbackSmoke.class",
            "dev/slinkywinky/attach/OriginalInputOwnershipAudit.class",
            "dev/slinkywinky/attach/OriginalInputOwnershipSmoke.class",
            "dev/slinkywinky/attach/R112KeyboardTarget.class",
        }
        if not required.issubset(entries) or entries.intersection(forbidden):
            raise AssertionError("R119 runtime JAR input-owner/fixture separation broken")
    print("PASS: R119 test-only keyboard fixtures absent from instrument JAR")
    render_audit_jar = loader.parent / "slinkywinky_render_audit.jar"
    with zipfile.ZipFile(render_audit_jar, "r") as source_audit:
        audit_entries = set(source_audit.namelist())
        if not {
            "dev/slinkywinky/attach/R162InputOrderLedger.class",
            "dev/slinkywinky/attach/R162InputOrderLedgerSmoke.class",
        }.issubset(audit_entries):
            raise AssertionError("R162 offline source audit missing from audit JAR")
    if ("dev/slinkywinky/attach/R162InputOrderLedger.class" in entries or
        "dev/slinkywinky/attach/R162InputOrderLedgerSmoke.class" in entries):
        raise AssertionError("R162 test-only source inspector leaked into agent")
    print("PASS: R162 source ledger belongs only to the read-only audit JAR")
    with zipfile.ZipFile(render_audit_jar, "r") as source_audit:
        contents = set(source_audit.namelist())
        required_r163 = {
            "dev/slinkywinky/attach/R163InputControlGraph.class",
            "dev/slinkywinky/attach/R163InputControlGraphSmoke.class",
        }
        if not required_r163.issubset(contents):
            raise AssertionError("R163 CFG analyzer missing from audit JAR")
        if entries.intersection(required_r163):
            raise AssertionError("R163 test-only control-flow code leaked into agent")
    print("PASS: R163 original-game input CFG tests excluded from runtime agent")
    r164_source_only = {
        "dev/slinkywinky/attach/R164OfflineModalGate.class",
        "dev/slinkywinky/attach/R164OfflineInputCut.class",
        "dev/slinkywinky/attach/R164OfflineInputCutSmoke.class",
    }
    with zipfile.ZipFile(render_audit_jar, "r") as read_only_audit:
        if not r164_source_only.issubset(set(read_only_audit.namelist())):
            raise AssertionError("R164 offline gate tests missing from audit JAR")
    if entries.intersection(r164_source_only):
        raise AssertionError("R164 offline input gate leaked into injected agent")
    print("PASS: R164 method-isolation gate is confined to offline audit JAR")
    classpath = str(classes) + ";" + str(instrument_jar)
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
    # R64: invoke ASM verifier on generated classfile fixtures only. No
    # Mojang binary, Minecraft launch, render injection or graphics context.
    render_audit = subprocess.run(
        [str(java), "-cp", classpath,
         "dev.slinkywinky.attach.RenderCallsiteAuditSmoke"],
        capture_output=True, text=True, env=env, timeout=15)
    if (render_audit.returncode != 0 or
        "R64_RENDER_CALLSITE_NEGATIVE_CONTROLS_PASS" not in render_audit.stdout):
        raise AssertionError("Render mapping admission negative controls failed: " +
                             render_audit.stdout + render_audit.stderr)
    print("PASS: offline renderer callsite uniqueness and wrong-target rejection")
    # R107: inspect generated bytecode only. This does not read key events
    # or create any hook/consumer in a Minecraft process.
    keyboard_audit = subprocess.run(
        [str(java), "-cp", classpath,
         "dev.slinkywinky.attach.KeyboardInputCallsiteAuditSmoke"],
        capture_output=True, text=True, env=env, timeout=15)
    if (keyboard_audit.returncode != 0 or
        "R107_KEYBOARD_EVENT_PATH_SYNTHETIC_NEGATIVE_CONTROLS_PASS"
        not in keyboard_audit.stdout):
        raise AssertionError("R107 keyboard path negative controls failed: " +
                             keyboard_audit.stdout + keyboard_audit.stderr)
    print("PASS: R107 original keyboard path source-shape negative controls")
    # R119: deeper, independent full input-method inventory. Reject
    # queue-only 'modal safety' when game keybindings are still reachable.
    # Synthetic bytecode only; full SHA-pinned vanilla jar tested separately.
    input_owner = subprocess.run(
        [str(java), "-cp", classpath,
         "dev.slinkywinky.attach.OriginalInputOwnershipSmoke"],
        capture_output=True, text=True, env=env, timeout=15)
    if (input_owner.returncode != 0 or
        "R119_SYNTHETIC_PRE_AND_POST_KEYBINDING_MODAL_DENIAL_PASS"
        not in input_owner.stdout or
        "R119_UNVERIFIED_ORIGINAL_AND_MALFORMED_CLASS_DENIAL_PASS"
        not in input_owner.stdout):
        raise AssertionError("R119 full input ownership negative gate failed: " +
                             input_owner.stdout + input_owner.stderr)
    print("PASS: R119 residual game-keybinding modal guard, fixture only")
    # R109: a second offline ASM-only guard proves that a keyboard queue
    # callsite may be substituted in memory with NO other opcode changes.
    # Nothing here is installed in the actual game or instrumentation JAR.
    keyboard_weave = subprocess.run(
        [str(java), "-cp", classpath,
         "dev.slinkywinky.attach.OriginalKeyboardLoopWeaveSmoke"],
        capture_output=True, text=True, env=env, timeout=15)
    if (keyboard_weave.returncode != 0 or
        "R109_SYNTHETIC_KEYBOARD_LOOP_WEAVE_ONE_CHANGE_AND_NEGATIVE_CONTROLS_PASS"
        not in keyboard_weave.stdout):
        raise AssertionError("R109 offline keyboard ASM negative controls failed: " +
                             keyboard_weave.stdout + keyboard_weave.stderr)
    print("PASS: R109 memory-only original input callsite rewrite controls")
    # R110: functional Java 8 LWJGL2 event-owner gate exercised against a
    # disposable mock keyboard, with no Minecraft or original-game input hook.
    queue_owner = subprocess.run(
        [str(java), "-cp", classpath,
         "dev.slinkywinky.attach.KeyboardQueueOwnerSmoke"],
        capture_output=True, text=True, env=env, timeout=15)
    if (queue_owner.returncode != 0 or
        "R110_MOCK_LWJGL_VANILLA_PASSTHROUGH_AND_MODAL_QUEUE_QUARANTINE_PASS"
        not in queue_owner.stdout or
        "R110_KEY_DOWN_UP_REPEATS_CLOSE_DRAIN_AND_RESTORE_GUARDS_PASS"
        not in queue_owner.stdout):
        raise AssertionError("R110 Java 8 input-owner isolated regression failed: " +
                             queue_owner.stdout + queue_owner.stderr)
    print("PASS: R110 independent Java 8 modal queue owner, no live game hook")
    # R118: key codes are routed into a strict first-party preview-only
    # PageUp/PageDown sink using the SAME mock LWJGL2 owner. No original
    # Minecraft callsite, OS hook, JNI page dispatch or gameplay input.
    page_relay = subprocess.run(
        [str(java), "-cp", classpath,
         "dev.slinkywinky.attach.KeyboardPageRelaySmoke"],
        capture_output=True, text=True, env=env, timeout=15)
    if (page_relay.returncode != 0 or
        "R118_MOCK_JAVA8_PAGE_UP_DOWN_TO_FIRST_PARTY_PREVIEW_SINK_PASS"
        not in page_relay.stdout or
        "R118_EXCLUSIVE_EPOCH_REPEAT_OWNER_CLOSE_AND_VANILLA_RESTORE_PASS"
        not in page_relay.stdout):
        raise AssertionError("R118 Java 8 page-relay regression failed: " +
                             page_relay.stdout + page_relay.stderr)
    print("PASS: R118 Java 8 source-owned page relay, dormant, no live input")
    # R112 adds a real Java 8 classloader execution test for the same
    # single-call rewrite primitive used by the R111 SHA-pinned owner.
    # Only the first-party synthetic target and fake LWJGL2 are executable.
    executable_bridge = subprocess.run(
        [str(java), "-cp", classpath,
         "dev.slinkywinky.attach.R112KeyboardBridgeSmoke"],
        capture_output=True, text=True, env=env, timeout=20)
    if (executable_bridge.returncode != 0 or
        "R112_SYNTHETIC_TRANSFORMED_CLASS_EXECUTION_AND_KEY_OWNER_PASS"
        not in executable_bridge.stdout or
        "R112_EXACT_ONE_CALLSITE_MODAL_NO_QUEUE_PASSTHROUGH_FIXTURE_PASS"
        not in executable_bridge.stdout or
        "R119_ONE_CALLSITE_MODAL_EXECUTES_PRE_AND_POST_LOGIC_FIXTURE_PASS"
        not in executable_bridge.stdout):
        raise AssertionError("R112 executable isolated keyboard bridge failed: " +
                             executable_bridge.stdout + executable_bridge.stderr)
    print("PASS: R112 executed bytecode bridge and original queue delivery guards")
    # R72: generated bfk class only; source and transformed bytes stay in
    # the disposable JVM heap. The real Mojang JAR is tested separately in CI.
    offline_weave = subprocess.run(
        [str(java), "-cp", classpath,
         "dev.slinkywinky.attach.OriginalRendererWeaveSmoke"],
        capture_output=True, text=True, env=env, timeout=15)
    if (offline_weave.returncode != 0 or
        "R72_OFFLINE_WEAVE_SYNTHETIC_NEGATIVE_CONTROLS_PASS"
        not in offline_weave.stdout):
        raise AssertionError("Offline renderer ASM preflight controls failed: " +
                             offline_weave.stdout + offline_weave.stderr)
    print("PASS: R72 original-renderer dry-run weave safety on synthetic classes")
    # R73 passive live-loaded renderer fingerprint verifier; this isolated JVM
    # provides ONLY synthetic ASM fixtures and refuses unrelated game classes.
    render_preflight = subprocess.run(
        [str(java), "-cp", classpath,
         "dev.slinkywinky.attach.RendererLivePreflightSmoke"],
        capture_output=True, text=True, env=env, timeout=15)
    if (render_preflight.returncode != 0 or
        "R73_RENDERER_PREFLIGHT_SYNTHETIC_GUARDS_PASS" not in render_preflight.stdout or
        "R73_UNRELATED_JVM_REJECTED_BEFORE_TRANSFORM_PASS" not in render_preflight.stdout):
        raise AssertionError("R73 passive renderer admission regression failed: " +
                             render_preflight.stdout + render_preflight.stderr)
    print("PASS: R73 exact loaded renderer fingerprint and shape denials")
    # R75: finite owner-callsite gate must reject invalid samples and unknown
    # renderer class bytes. No game or graphics process is involved.
    r75_gate = subprocess.run(
        [str(java), "-cp", classpath,
         "dev.slinkywinky.attach.RenderFrameGateSmoke"],
        capture_output=True, text=True, env=env, timeout=15)
    if (r75_gate.returncode != 0 or
        "R75_RENDER_OWNER_STACK_AND_EIGHT_FRAME_LIMIT_PASS" not in r75_gate.stdout or
        "R75_CROSS_THREAD_INVALID_INPUT_AND_STOP_FAIL_CLOSED_PASS" not in r75_gate.stdout or
        "R75_ORIGINAL_RENDERER_HASH_NEGATIVE_CONTROL_PASS" not in r75_gate.stdout):
        raise AssertionError("R75 passive owner-callsite gate failed: " +
                             r75_gate.stdout + r75_gate.stderr)
    print("PASS: R75 original render-callsite owner, cap and rejection safeguards")
    # R65: bounded, read-only render callback test in generated classfiles.
    # The production JVM agent is intentionally not involved.
    render_callback = subprocess.run(
        [str(java), "-cp", classpath,
         "dev.slinkywinky.attach.RenderCallbackSmoke"],
        capture_output=True, text=True, env=env, timeout=15)
    if (render_callback.returncode != 0 or
        "R65_READ_ONLY_RENDER_FIXTURE_PASS" not in render_callback.stdout or
        "R65_RENDER_THREAD_AND_BOUNDS_NEGATIVE_CONTROLS_PASS" not in render_callback.stdout):
        raise AssertionError("Read-only render fixture admission failed: " +
                             render_callback.stdout + render_callback.stderr)
    print("PASS: isolated bounded render callback and wrong-thread rejection")
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
    # R73 must not be accessible without a separate, explicit operator gate.
    no_render_gate = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--render-preflight"],
        capture_output=True, text=True, env=env, timeout=15)
    if (no_render_gate.returncode == 0 or
        "SLINKYWINKY_RENDER_PREFLIGHT" not in no_render_gate.stderr):
        raise AssertionError("Render preflight bypassed explicit opt-in")
    render_env=production_env.copy()
    render_env["SLINKYWINKY_RENDER_PREFLIGHT"]="1"
    render_wrong_target = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--render-preflight"],
        capture_output=True, text=True, env=render_env, timeout=15)
    if (render_wrong_target.returncode == 0 or
        "Refusing non-Minecraft target" not in render_wrong_target.stderr):
        raise AssertionError("Render preflight admitted a non-Minecraft process")
    print("PASS: R73 operator opt-in enforced and unrelated JVM refused")
    # R75 has an entirely separate exact-frame operator opt-in; neither a
    # test PID nor a production non-Minecraft PID can pass through the CLI.
    no_r75_gate = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--render-callback=8"],
        capture_output=True, text=True, env=env, timeout=15)
    if no_r75_gate.returncode == 0 or (
        "SLINKYWINKY_RENDER_CALLBACK" not in no_r75_gate.stderr):
        raise AssertionError("R75 live callback bypassed separate opt-in")
    r75_env = production_env.copy()
    r75_env["SLINKYWINKY_RENDER_CALLBACK"] = "1"
    wrong_target = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--render-callback=8"],
        capture_output=True, text=True, env=r75_env, timeout=15)
    if wrong_target.returncode == 0 or (
        "Refusing non-Minecraft target" not in wrong_target.stderr):
        raise AssertionError("R75 live callback admitted a non-Minecraft JVM")
    test_bypass_env = env.copy()
    test_bypass_env["SLINKYWINKY_RENDER_CALLBACK"] = "1"
    bypass = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--render-callback=8"],
        capture_output=True, text=True, env=test_bypass_env, timeout=15)
    if bypass.returncode == 0 or (
        "cannot run in Attach test mode" not in bypass.stderr):
        raise AssertionError("R75 original renderer allowed synthetic Attach bypass")
    print("PASS: R75 explicit gate, production PID refusal and test-bypass refusal")

    # R78 native WGL read-only inspection requires TWO independent explicit
    # gates and must never attach to a non-Minecraft or synthetic test target.
    r78_denied = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--render-context=8"],
        capture_output=True, text=True, env=env, timeout=15)
    if (r78_denied.returncode == 0 or
        "SLINKYWINKY_RENDER_CONTEXT" not in r78_denied.stderr):
        raise AssertionError("R78 bypassed dual explicit operator gate")
    r78_test = env.copy()
    r78_test["SLINKYWINKY_RENDER_CONTEXT"] = "1"
    r78_test["SLINKYWINKY_RENDER_CALLBACK"] = "1"
    r78_wrong = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--render-context=8"],
        capture_output=True, text=True, env=r78_test, timeout=15)
    if (r78_wrong.returncode == 0 or
        "cannot run in Attach test mode" not in r78_wrong.stderr):
        raise AssertionError("R78 admitted synthetic Attach target")
    r78_production = production_env.copy()
    r78_production["SLINKYWINKY_RENDER_CONTEXT"] = "1"
    r78_production["SLINKYWINKY_RENDER_CALLBACK"] = "1"
    r78_non_game = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--render-context=8"],
        capture_output=True, text=True, env=r78_production, timeout=15)
    if (r78_non_game.returncode == 0 or
        "Refusing non-Minecraft target" not in r78_non_game.stderr):
        raise AssertionError("R78 admitted a non-Minecraft process")
    print("PASS: R78 double operator gate, synthetic and non-Minecraft PID denial")


    # The unverified Minecraft tick hook must be explicitly developer-enabled.
    print("R61_TEST_PHASE=initial-hook-denial", flush=True)
    no_gate = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--hook-probe"],
        capture_output=True, text=True, env=env, timeout=15)
    if no_gate.returncode == 0 or "SLINKYWINKY_EXPERIMENTAL_HOOK" not in no_gate.stderr:
        raise AssertionError("Unverified tick hook must be disabled by default")
    hook_env = env.copy()
    hook_env["SLINKYWINKY_EXPERIMENTAL_HOOK"] = "1"
    print("R61_TEST_PHASE=initial-20-hook", flush=True)
    hook = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--hook-probe"],
        capture_output=True, text=True, env=hook_env, timeout=22)
    if hook.returncode != 0 or "mode=hook-probe" not in hook.stdout:
        raise AssertionError("Read-only tick hook installation failed: " +
                             hook.stdout + hook.stderr)
    # The original 20-tick default must finish before a second admission.
    time.sleep(1.8)
    print("R61_TEST_PHASE=bounded-120-denial", flush=True)
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
    print("R61_TEST_PHASE=bounded-120-start", flush=True)
    extended = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--hook-probe=120"],
        capture_output=True, text=True, env=extended_env, timeout=22)
    if extended.returncode != 0 or "mode=hook-probe" not in extended.stdout:
        raise AssertionError("Bounded extended native hook refused: " +
                             extended.stdout + extended.stderr)
    # AttachTarget advances one game tick per ~30 ms. Permit 120 observed ticks
    # plus asynchronous class fingerprint restoration before termination.
    time.sleep(5.0)
    print("R61_TEST_PHASE=no-active-stop", flush=True)
    # R61: a stop with no active continuous session must fail closed.
    no_session_stop = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--stop-observation"],
        capture_output=True, text=True, env=env, timeout=15)
    if no_session_stop.returncode == 0:
        raise AssertionError("Stop must not succeed without continuous ownership")
    print("R61_TEST_PHASE=continuous-gate", flush=True)
    no_continuous_gate = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--hook-probe=continuous"],
        capture_output=True, text=True, env=hook_env, timeout=15)
    if (no_continuous_gate.returncode == 0 or
        "SLINKYWINKY_CONTINUOUS_OBSERVATION" not in no_continuous_gate.stderr):
        raise AssertionError("Continuous session bypassed its separate opt-in")
    continuous_env = hook_env.copy()
    continuous_env["SLINKYWINKY_CONTINUOUS_OBSERVATION"] = "1"
    if target.poll() is not None:
        raise AssertionError("Disposable JVM died before continuous attach")
    print("R61_TEST_PHASE=continuous-start", flush=True)
    continuous = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--hook-probe=continuous"],
        capture_output=True, text=True, env=continuous_env, timeout=22)
    if continuous.returncode != 0 or "mode=hook-probe" not in continuous.stdout:
        raise AssertionError("Continuous read-only start refused: " +
                             continuous.stdout + continuous.stderr)
    # Let the disposable target advance well beyond the default 20 callbacks,
    # but do NOT wait for the 36,000-tick independent upper bound.
    time.sleep(1.3)
    print("R61_TEST_PHASE=explicit-stop", flush=True)
    stop = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--stop-observation"],
        capture_output=True, text=True, env=env, timeout=22)
    if stop.returncode != 0 or "mode=stop-observation" not in stop.stdout:
        raise AssertionError("Explicit continuous stop command failed: " +
                             stop.stdout + stop.stderr)
    time.sleep(1.8)
    print("R61_TEST_PHASE=post-cleanup-denial", flush=True)
    duplicate_stop = subprocess.run(
        [str(loader), "attach", str(target.pid), str(home), "--stop-observation"],
        capture_output=True, text=True, env=env, timeout=15)
    if duplicate_stop.returncode == 0:
        raise AssertionError("Duplicate stop must reject after native cleanup")
    print("R61_TEST_PHASE=target-termination", flush=True)
    target.terminate()
    tail, _ = target.communicate(timeout=5)
    for marker in ("OBSERVATION_RELEASE_PASS", "NATIVE_SESSION_OPEN_PASS", "NATIVE_SESSION_CLOSE_PASS", "OBSERVATION_MAPPING_ADMITTED", "NATIVE_OBSERVATION_PASS world=1 player=1 target=1", "NATIVE_CXX_PREFLIGHT_PASS", "NATIVE_GAME_THREAD_CXX_PASS", "GAME_THREAD_TICK_ENTRY_PASS", "GAME_THREAD_TICK_20_PASS ticks=20", "DIAGNOSTIC_TICK_LIMIT=120",
                   "GAME_THREAD_TICK_120_PASS ticks=120",
                   "CLASS_BYTECODE_RESTORE_PASS", "GAME_THREAD_HOOK_CLEANUP_PASS"):
        if marker not in tail:
            raise AssertionError("Missing tick hook or restoration marker " + marker +
                                 ": " + tail[-1600:])
    if tail.count("CLASS_BYTECODE_RESTORE_PASS") != 3 or tail.count("GAME_THREAD_HOOK_CLEANUP_PASS") != 3:
        raise AssertionError("Bounded and stop-driven sessions must restore exact class bytes three times")
    if tail.count("CONTINUOUS_STOP_ON_CLIENT_THREAD_PASS") != 1:
        raise AssertionError(
            "Native loader acknowledged Win32 event but the client-thread owner "
            "did not complete the stop: " + tail[-2800:])
    if "GAME_THREAD_TICK_36000_PASS" in tail or "GAME_THREAD_HOOK_ABORT" in tail:
        raise AssertionError("Stop-driven session reached cap or aborted unexpectedly")
    # R60: inspect only frame counts. Never trust a world/target marker from a
    # rejected native frame, or treat a mock ray hit as attack eligibility.
    import re
    summaries = re.findall(
        r"NATIVE_OBSERVATION_SUMMARY accepted=(\d+) worldPlayer=(\d+) "
        r"rayEntity=(\d+) generations=(\d+)", tail)
    if len(summaries) != 3 or summaries[:2] != [
        ("20", "20", "20", "0"), ("120", "120", "120", "0")
    ]:
        raise AssertionError("Expected two exact bounded plus one continuous summary: " +
                             repr(summaries))
    continuous_count = int(summaries[2][0])
    if (not 21 <= continuous_count < 36000 or
        summaries[2] != (str(continuous_count), str(continuous_count),
                         str(continuous_count), "0")):
        raise AssertionError("Continuous accepted-frame summary inconsistent: " +
                             repr(summaries[2]))
    if tail.count("NATIVE_WORLD_PLAYER_OBSERVED tick=1") != 3 or (
        tail.count("NATIVE_RAY_ENTITY_OBSERVED tick=1") != 3):
        raise AssertionError("Per-attachment world and ray markers were not reset")
    print("PASS: 20, 120, and stop-driven continuous native observation; restored bytecode x3")

    print("PASS: standalone Windows loader -> Java 8 Attach API -> native JVMTI agent")
finally:
    if target.poll() is None:
        target.terminate()
    try:
        target.wait(timeout=5)
    except subprocess.TimeoutExpired:
        target.kill()
        target.wait(timeout=3)
