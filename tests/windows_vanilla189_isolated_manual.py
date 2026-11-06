#!/usr/bin/env python3
"""Manual-only, isolated, original Minecraft 1.8.9 read-only injector acceptance.

Requires --run and a previously installed original vanilla 1.8.9 client.
Never downloads Minecraft files, logs into a real account, or starts via Forge.
It does not modify the normal .minecraft game directory or unrelated JVMs.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys
import time
import zipfile

PINNED_CLIENT_SHA256 = "14f0d96d1a56fb4f5c3b2233d00699525893fe5ce3dcf181e7de59120595d298"
MARKERS = (
    "CALLBACK_CLASSLOADER_VALIDATED",
    "OBSERVATION_MAPPING_ADMITTED",
    "NATIVE_SESSION_OPEN_PASS",
    "NATIVE_OBSERVATION_PASS",
    "NATIVE_GAME_THREAD_CXX_PASS",
    "GAME_THREAD_TICK_20_PASS",
    "NATIVE_SESSION_CLOSE_PASS",
    "OBSERVATION_RELEASE_PASS",
    "CLASS_BYTECODE_RESTORE_PASS",
    "GAME_THREAD_HOOK_CLEANUP_PASS",
)


def digest(path, algorithm):
    h = hashlib.new(algorithm)
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def accepted_rules(library):
    """Apply the vanilla JSON allow/disallow rules, excluding unsupported OS rules."""
    rules = library.get("rules", [])
    if not rules:
        return True
    allow = False
    for rule in rules:
        os_rule = rule.get("os", {})
        if os_rule.get("name", "windows") != "windows":
            continue
        if "version" in os_rule:  # No guessed regex Windows compatibility
            continue
        allow = rule["action"] == "allow"
    return allow


def installed_artifact(minecraft, metadata):
    candidate = minecraft / "libraries" / metadata["path"]
    if not candidate.is_file():
        raise RuntimeError("Missing locally installed official library: " + metadata["path"])
    sha1 = metadata.get("sha1")
    if sha1 and digest(candidate, "sha1") != sha1.lower():
        raise RuntimeError("Official local library integrity mismatch: " + metadata["path"])
    return candidate


def attachable_games(loader, jdk):
    response = subprocess.run(
        [str(loader), "list", str(jdk)], capture_output=True, text=True, timeout=15
    )
    if response.returncode:
        raise RuntimeError("Could not enumerate attachable JVMs: " + response.stderr)
    games = []
    for line in response.stdout.splitlines():
        match = re.match(r"^([0-9]+)\s+(.+)$", line)
        if match and any(s in match.group(2) for s in (
            "GradleStart", "net.minecraft.client.main.Main",
            "net.minecraft.launchwrapper.Launch",
        )):
            games.append((int(match.group(1)), match.group(2)))
    return games


def original_render_classes_loaded(log_text):
    """Only exact Java 8 class-load receipts from original 1.8.9.jar count."""
    if "LWJGL Version:" not in log_text:
        return False
    for name in ("ave", "bfk"):
        pattern = r"(?m)^\[Loaded " + name + (
            r" from file:/[^\r\n]+/1\.8\.9\.jar\]\r?$")
        if re.search(pattern, log_text) is None:
            return False
    return True


def validate_observation_evidence(log_text, attempts, expected_ticks,
                                  require_world=False, require_ray_entity=False):
    """Validate original-process markers; never infer a world from attachment alone."""
    if any(marker in log_text for marker in (
        "GAME_THREAD_HOOK_ABORT", "NATIVE_GAME_THREAD_PROBE_REJECTED",
        "GAME_THREAD_HOOK_CLEANUP_FAIL", "NATIVE_SESSION_CLOSE_REJECTED"
    )):
        raise RuntimeError("Game hook rejection, native close, or restoration failure")
    evidence = re.findall(
        r"NATIVE_OBSERVATION_SUMMARY accepted=(\d+) worldPlayer=(\d+) "
        r"rayEntity=(\d+) generations=(\d+)", log_text
    )
    if len(evidence) != attempts:
        raise RuntimeError("Expected exactly one accepted-frame summary per attachment")
    for accepted, world_frames, ray_frames, generations in evidence:
        accepted, world_frames, ray_frames, generations = map(
            int, (accepted, world_frames, ray_frames, generations)
        )
        if expected_ticks is None:
            if not 21 <= accepted < 36000:
                raise RuntimeError("Stop-driven accepted tick count outside safe range")
        elif accepted != expected_ticks:
            raise RuntimeError("Unexpected native-accepted callback count")
        if world_frames > accepted or ray_frames > world_frames or generations >= accepted:
            raise RuntimeError("Inconsistent native-accepted observation counters")
        if require_world and world_frames == 0:
            raise RuntimeError("No native-accepted world+player frames: menu-only is not world acceptance")
        if require_ray_entity and ray_frames == 0:
            raise RuntimeError("No native-accepted ray-selected entity frames")
    fingerprints = re.findall(
        r"CLASS_BYTECODE_RESTORE_PASS sha256=([a-f0-9]{64})", log_text
    )
    if len(fingerprints) != attempts or len(set(fingerprints)) != 1:
        raise RuntimeError("Original class fingerprint not restored for every attachment")
    if log_text.count("GAME_THREAD_HOOK_CLEANUP_PASS") != attempts:
        raise RuntimeError("Missing successful hook cleanup for an attachment")
    return evidence, fingerprints


def main():
    parser = argparse.ArgumentParser(
        description="Original vanilla 1.8.9 native-injector manual acceptance"
    )
    parser.add_argument("--workspace", type=Path, required=True,
                        help="Separate workspace containing the built native injector")
    parser.add_argument("--jdk8", type=Path, required=True,
                        help="Installed full x64 Java 8 JDK")
    parser.add_argument("--loader", type=Path,
                        help="Explicit built slinkywinky_loader.exe path inside --workspace; "
                             "required if several build directories exist")
    parser.add_argument("--minecraft-dir", type=Path,
                        default=Path(os.environ.get("APPDATA", ".")) / ".minecraft",
                        help="Existing official Minecraft installation (read-only)")
    parser.add_argument("--run", action="store_true",
                        help="Explicitly authorize launching one disposable vanilla client")
    parser.add_argument("--render-window-audit-acceptance", action="store_true",
                        help="R103 opt-in original renderer HDC-to-HWND window/foreground audit plus R102 external check")
    parser.add_argument("--window-identity-acceptance", action="store_true",
                        help="R102 no-input: verify unique owned original game window and foreground/background identity")
    parser.add_argument("--render-operator-menu-acceptance", action="store_true",
                        help="R100 isolated real 1.8.9 externally acknowledged menu navigation and stop")
    parser.add_argument("--render-retained-acceptance", action="store_true",
                        help="R99 opt-in: 64 original-game retained GL11 frames and inert navigation verification")
    parser.add_argument("--render-key-acceptance", action="store_true",
                        help="R95 opt-in: exact own-game window SendInput Insert x2 plus native stop, not physical hardware")
    parser.add_argument("--render-manual-acceptance", action="store_true",
                        help="R94 opt-in: supervised original client with three PID-scoped toggles and stop")
    parser.add_argument("--render-visibility-acceptance", action="store_true",
                        help="R93 opt-in: compact show-20 / hide-16 / show-28, then restore")
    parser.add_argument("--render-multiframe-acceptance", action="store_true",
                        help="Opt-in R92: exactly 8 WGL preflight plus 64 native menu frames")
    parser.add_argument("--render-overlay-acceptance", action="store_true",
                        help="Opt-in R91: isolated one-frame native menu and restored GL state")
    parser.add_argument("--render-context-acceptance", action="store_true",
                        help="Opt-in R90: isolated vanilla process with bounded R81 renderer diagnostics; never draw the menu")
    parser.add_argument("--world-observation", action="store_true",
                        help="Manual-only: enter a local single-player world, then run a 600-tick read-only probe")
    parser.add_argument("--require-ray-entity", action="store_true",
                        help="Also require a native-accepted entity under the crosshair; implies --world-observation")
    parser.add_argument("--continuous-observation", action="store_true",
                        help="Opt-in R61 manual world session; stop after a bounded observation interval")
    args = parser.parse_args()
    if not args.run:
        parser.error("Refusing to start Minecraft: pass --run explicitly")
    if os.name != "nt":
        parser.error("Windows x64 only")
    if sum((args.render_context_acceptance,args.render_overlay_acceptance,
            args.render_multiframe_acceptance,args.render_visibility_acceptance,
            args.render_manual_acceptance,args.render_key_acceptance,
            args.render_retained_acceptance,
            args.render_operator_menu_acceptance,
            args.window_identity_acceptance,
            args.render_window_audit_acceptance))>1:
        parser.error("R90/R91/R92/R93/R94/R95/R99/R100/R102/R103 acceptance modes are mutually exclusive")
    if (args.render_context_acceptance or args.render_overlay_acceptance or args.render_multiframe_acceptance or args.render_visibility_acceptance or args.render_manual_acceptance or args.render_key_acceptance or args.render_retained_acceptance or args.render_operator_menu_acceptance or args.render_window_audit_acceptance) and (args.world_observation or
                                           args.require_ray_entity or
                                           args.continuous_observation):
        parser.error("R90 renderer acceptance must run alone, not with world/tick probes")
    if args.require_ray_entity and not args.world_observation:
        parser.error("--require-ray-entity requires --world-observation")
    if args.continuous_observation and not args.world_observation:
        parser.error("--continuous-observation requires --world-observation")
    if args.world_observation and not sys.stdin.isatty():
        parser.error("Manual world admission requires an interactive terminal")

    game_root = args.minecraft_dir.resolve()
    workspace = args.workspace.resolve()
    jdk = args.jdk8.resolve()
    if workspace == game_root or game_root in workspace.parents:
        parser.error("Workspace must NOT be inside the original .minecraft")
    if not (jdk / "lib" / "tools.jar").is_file():
        parser.error("Java 8 full JDK with tools.jar required")

    client_dir = game_root / "versions" / "1.8.9"
    client_jar = client_dir / "1.8.9.jar"
    manifest_path = client_dir / "1.8.9.json"
    if args.loader:
        launcher = args.loader.resolve()
    else:
        choices = [
            workspace / folder / "Release" / "slinkywinky_loader.exe"
            for folder in ("build-inject", "build-native-agent", "build")
        ]
        found = [candidate for candidate in choices if candidate.is_file()]
        if len(found) != 1:
            parser.error("Expected one built loader in workspace; use --loader to select "
                         "an exact executable when none or multiple are found")
        launcher = found[0].resolve()
    if workspace not in launcher.parents or launcher.name.lower() != "slinkywinky_loader.exe":
        parser.error("Loader must be the named native executable inside the workspace")
    if not (client_jar.is_file() and manifest_path.is_file() and launcher.is_file()):
        parser.error("Missing installed vanilla 1.8.9 JAR, JSON, or standalone loader")
    if digest(client_jar, "sha256") != PINNED_CLIENT_SHA256:
        parser.error("Unrecognized Minecraft client: pinned SHA-256 mismatch")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("id") != "1.8.9" or manifest.get("mainClass") != "net.minecraft.client.main.Main":
        parser.error("Expected original vanilla 1.8.9 launcher metadata")
    if manifest.get("assets") != "1.8":
        parser.error("Unexpected asset index")

    classpath = []
    native_libraries = {}
    for library in manifest["libraries"]:
        if not accepted_rules(library):
            continue
        downloads = library.get("downloads", {})
        main_artifact = downloads.get("artifact")
        if main_artifact and not library.get("natives"):
            classpath.append(str(installed_artifact(game_root, main_artifact)))
        native_key = library.get("natives", {}).get("windows")
        if not native_key:
            continue
        native_artifact = downloads.get("classifiers", {}).get(native_key)
        if not native_artifact:
            continue
        path = installed_artifact(game_root, native_artifact)
        with zipfile.ZipFile(path) as archive:
            if archive.testzip() is not None:
                raise RuntimeError("Corrupt native library archive: " + str(path))
            for item in archive.namelist():
                if not item.lower().endswith(".dll"):
                    continue
                filename = PurePosixPath(item).name
                content = archive.read(item)
                previous = native_libraries.setdefault(filename, content)
                if previous != content:
                    raise RuntimeError("Conflicting bundled native library: " + filename)
    if "lwjgl64.dll" not in {s.lower() for s in native_libraries}:
        raise RuntimeError("Required x64 LWJGL DLL missing")
    assets_dir = game_root / "assets"
    if not (assets_dir / "indexes" / "1.8.json").is_file():
        raise RuntimeError("Locally installed 1.8 asset index is missing")
    classpath.append(str(client_jar))

    # Refuse even a transient overlap with an existing game; never attach to it.
    if attachable_games(launcher, jdk):
        raise RuntimeError("Another Minecraft-like JVM exists: aborting")

    sandbox = workspace / "vanilla189-isolated"
    game_dir = sandbox / "game"
    native_dir = sandbox / "natives"
    game_dir.mkdir(parents=True, exist_ok=True)
    native_dir.mkdir(parents=True, exist_ok=True)
    for filename, content in native_libraries.items():
        (native_dir / filename).write_bytes(content)
    (game_dir / "options.txt").write_text(
        "fullscreen:false\nfancyGraphics:false\nrenderDistance:2\n", encoding="ascii"
    )
    log = sandbox / (
        ("r103-wgl-window-" if args.render_window_audit_acceptance else
         ("r100-operator-" if args.render_operator_menu_acceptance else
         ("r99-retained-" if args.render_retained_acceptance else
         ("r95-key-" if args.render_key_acceptance else
         ("r94-manual-" if args.render_manual_acceptance else
         ("r93-visibility-" if args.render_visibility_acceptance else
         ("r92-multiframe-" if args.render_multiframe_acceptance else
         ("r91-overlay-" if args.render_overlay_acceptance else "r90-render-")))))))) +
        str(time.time_ns()) + ".log"
        if (args.render_context_acceptance or args.render_overlay_acceptance or args.render_multiframe_acceptance or args.render_visibility_acceptance or args.render_manual_acceptance or args.render_key_acceptance or args.render_retained_acceptance or args.render_operator_menu_acceptance or args.render_window_audit_acceptance) else "vanilla-run.log"
    )
    if args.window_identity_acceptance:
        log=sandbox / ("r102-window-"+str(time.time_ns())+".log")
    cmd = [
        str(jdk / "bin" / "java.exe"),
        *(["-verbose:class"] if (args.render_context_acceptance or args.render_overlay_acceptance or args.render_multiframe_acceptance or args.render_visibility_acceptance or args.render_manual_acceptance or args.render_key_acceptance or args.render_retained_acceptance or args.render_operator_menu_acceptance or args.render_window_audit_acceptance or args.window_identity_acceptance) else []),
        "-Xmx768M",
        "-Djava.library.path=" + str(native_dir),
        "-Dlog4j2.formatMsgNoLookups=true",
        "-cp", os.pathsep.join(classpath),
        "net.minecraft.client.main.Main",
        "--username", "SWOfflineTest", "--version", "1.8.9",
        "--gameDir", str(game_dir), "--assetsDir", str(assets_dir),
        "--assetIndex", "1.8", "--uuid", "00000000000000000000000000000042",
        "--accessToken", "0", "--userType", "legacy",
        "--versionType", "release", "--userProperties", "{}",
    ]
    environment = os.environ.copy()
    environment.pop("SLINKYWINKY_ATTACH_TEST", None)
    if args.render_context_acceptance or args.render_overlay_acceptance or args.render_multiframe_acceptance or args.render_visibility_acceptance or args.render_manual_acceptance or args.render_key_acceptance or args.render_retained_acceptance or args.render_operator_menu_acceptance or args.render_window_audit_acceptance:
        # Native gameplay hooks are not part of renderer admission.
        environment.pop("SLINKYWINKY_EXPERIMENTAL_HOOK", None)
    else:
        environment["SLINKYWINKY_EXPERIMENTAL_HOOK"] = "1"
    if args.world_observation:
        environment["SLINKYWINKY_EXTENDED_DIAGNOSTIC"] = "1"
    if args.continuous_observation:
        environment["SLINKYWINKY_CONTINUOUS_OBSERVATION"] = "1"

    def read_log():
        return log.read_text(encoding="utf-8", errors="replace")

    def loaded_exact_original_render_classes():
        return original_render_classes_loaded(read_log())


    def wait_for(process, marker, count, seconds):
        end = time.monotonic() + seconds
        while time.monotonic() < end and process.poll() is None:
            if read_log().count(marker) >= count:
                return True
            time.sleep(0.25)
        return False

    print("Launching one isolated vanilla 1.8.9 JVM; no Forge.", flush=True)
    with log.open("xb" if (args.render_context_acceptance or args.render_overlay_acceptance or args.render_multiframe_acceptance or args.render_visibility_acceptance or args.render_manual_acceptance or args.render_key_acceptance or args.render_retained_acceptance or args.render_operator_menu_acceptance or args.render_window_audit_acceptance) else "wb") as output:
        game = subprocess.Popen(
            cmd, cwd=str(sandbox), env=environment, stdout=output,
            stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NEW_PROCESS_GROUP
        )
        try:
            if not (wait_for(game, "Setting user:", 1, 50) or
                    wait_for(game, "LWJGL Version:", 1, 8)):
                raise RuntimeError("Vanilla client did not initialize")
            exact = [(pid, name) for pid, name in attachable_games(launcher, jdk)
                     if pid == game.pid]
            if len(exact) != 1:
                raise RuntimeError("The newly launched vanilla PID was not uniquely enumerated")
            if args.window_identity_acceptance:
                # No Attach or JNI call: independent passive Win32 window
                # inspection of the already source-pinned original JAR.
                from original_189_live_render_acceptance import process_birth_token
                from windows_r102_window_identity_evidence import (
                    observe_owned_game, WindowAuditError)
                birth=process_birth_token(game.pid)
                deadline=time.monotonic()+25
                evidence=None
                while time.monotonic()<deadline and game.poll() is None:
                    try:
                        if not loaded_exact_original_render_classes():
                            raise WindowAuditError("Original source-loaded classes not yet present")
                        evidence=observe_owned_game(game.pid,birth)
                        if process_birth_token(game.pid)!=birth:
                            raise RuntimeError("Original game process changed during inspection")
                        break
                    except WindowAuditError:
                        time.sleep(0.25)
                if evidence is None:
                    raise RuntimeError("R102 exact original game window not admitted in 25 seconds")
                report=sandbox / ("r102-window-evidence-"+str(game.pid)+".json")
                if report.exists():
                    raise RuntimeError("Refusing to overwrite existing original-game R102 evidence")
                evidence["original_jar_sha256"]=PINNED_CLIENT_SHA256
                evidence["exact_original_render_classes_loaded"]=True
                evidence["target_process_reused"]=False
                report.write_text(json.dumps(evidence,sort_keys=True,indent=2)+"\n",
                                  encoding="utf-8")
                print("R102_ORIGINAL_WINDOW_IDENTITY_PASS pid="+str(game.pid)+
                      " foreground="+str(evidence["foreground_samples"])+
                      " background="+str(evidence["background_samples"])+
                      " report="+str(report),flush=True)
                print("R102_WINDOW_IDENTITY_NOT_WGL_OR_INPUT_CAPTURE_CERTIFICATION",flush=True)
                return
            if args.render_context_acceptance or args.render_overlay_acceptance or args.render_multiframe_acceptance or args.render_visibility_acceptance or args.render_manual_acceptance or args.render_key_acceptance or args.render_retained_acceptance or args.render_operator_menu_acceptance or args.render_window_audit_acceptance:
                # A Java 8 renderer starts later than the LWJGL log line.
                # Require real JVM class-load receipts for both exact original
                # obfuscated classes before sending the first Attach.
                end = time.monotonic() + 50
                while (time.monotonic() < end and game.poll() is None and
                       not loaded_exact_original_render_classes()):
                    time.sleep(0.2)
                if not loaded_exact_original_render_classes():
                    raise RuntimeError("Original vanilla renderer classes were not loaded before first Attach")
                print("R90_ORIGINAL_189_RENDER_CLASSES_LOADED_PASS",
                      "pid=" + str(game.pid), flush=True)

                harness = Path(__file__).with_name("original_189_live_render_acceptance.py")
                if not harness.is_file():
                    raise RuntimeError("Missing in-repository original renderer receipt checker")
                report = sandbox / (("r103-window-receipts-" if args.render_window_audit_acceptance
                                     else ("r100-operator-receipts-" if args.render_operator_menu_acceptance
                                     else ("r99-retained-receipts-" if args.render_retained_acceptance
                                     else ("r95-key-receipts-" if args.render_key_acceptance
                                     else ("r94-manual-receipts-" if args.render_manual_acceptance
                                     else ("r93-visibility-receipts-" if args.render_visibility_acceptance
                                     else ("r92-multiframe-receipts-" if args.render_multiframe_acceptance
                                     else ("r91-overlay-receipts-" if args.render_overlay_acceptance
                                           else "r90-render-receipts-")))))))) + str(game.pid) + ".json")
                if report.exists():
                    raise RuntimeError("Refusing to overwrite existing R90 diagnostic evidence")
                command = [
                    sys.executable, str(harness),
                    "--loader", str(launcher), "--jdk8", str(jdk),
                    "--pid", str(game.pid), "--target-stdout-log", str(log),
                    "--original-client-jar", str(client_jar),
                    "--mode", ("window" if args.render_window_audit_acceptance else
                               ("operator" if args.render_operator_menu_acceptance else
                               ("retained" if args.render_retained_acceptance else
                               ("key" if args.render_key_acceptance else
                               ("manual" if args.render_manual_acceptance else
                               ("visibility" if args.render_visibility_acceptance else
                               ("multiframe" if args.render_multiframe_acceptance else
                               ("overlay" if args.render_overlay_acceptance else "context")))))))),
                    "--timeout-seconds", "45",
                    "--execute", "--report", str(report)
                ]
                # The R81 harness independently checks exact class hashes,
                # target PID birth, fresh JVM stdout, WGL ownership and the
                # original-bytecode restoration before each subsequent stage.
                result = subprocess.run(command, capture_output=True,
                                        text=True, timeout=210,
                                        env=environment)
                if result.returncode != 0 or (
                        "R81_ORIGINAL_RENDER_DIAGNOSTIC_RECEIPTS_PASS"
                        not in result.stdout):
                    raise RuntimeError(
                        "Supervised original renderer admission rejected: " +
                        (result.stderr + result.stdout)[-5000:])
                evidence = json.loads(report.read_text(encoding="utf-8"))
                if (len(evidence.get("stages", [])) !=
                        (7 if (args.render_manual_acceptance or args.render_key_acceptance or
                               args.render_operator_menu_acceptance) else
                         (6 if (args.render_visibility_acceptance or args.render_retained_acceptance) else
                         (5 if args.render_multiframe_acceptance else
                         (4 if (args.render_overlay_acceptance or args.render_window_audit_acceptance) else 3)))) or
                        evidence.get("in_game_menu_verified") is not False or
                        evidence.get("visual_game_acceptance") is not False):
                    raise RuntimeError("Unexpected renderer evidence scope")
                if game.poll() is not None:
                    raise RuntimeError("Vanilla JVM exited during renderer verification")
                if args.render_window_audit_acceptance:
                    print("R103_ORIGINAL_189_RENDER_WINDOW_RECEIPTS_PASS",
                          "pid=" + str(game.pid), "report=" + str(report),flush=True)
                    print("R103_READ_ONLY_NO_FOREGROUND_INPUT_OR_GAMEPLAY",flush=True)
                    return
                if args.render_operator_menu_acceptance:
                    print("R100_ORIGINAL_189_OPERATOR_MENU_RECEIPTS_PASS",
                          "pid=" + str(game.pid), "report=" + str(report),flush=True)
                    print("R100_EXTERNAL_OPERATOR_COMMANDS_NOT_GAME_KEYBOARD_OR_MOUSE",flush=True)
                    return
                if args.render_retained_acceptance:
                    print("R99_ORIGINAL_189_RETAINED_MENU_RECEIPTS_PASS",
                          "pid=" + str(game.pid), "report=" + str(report),flush=True)
                    print("R99_IN_GAME_RETAINED_RENDER_NOT_INTERACTIVE_OR_GAMEPLAY",flush=True)
                    return
                print(("R95_ORIGINAL_189_SYNTHETIC_INSERT_SOURCE_RECEIPTS_PASS"
                       if args.render_key_acceptance else
                       ("R94_ORIGINAL_189_OPERATOR_VISIBILITY_RECEIPTS_PASS"
                        if args.render_manual_acceptance else
                       ("R93_ORIGINAL_189_VISIBILITY_CYCLE_RECEIPTS_PASS"
                        if args.render_visibility_acceptance else
                       ("R92_ORIGINAL_189_MULTIFRAME_OVERLAY_RECEIPTS_PASS"
                        if args.render_multiframe_acceptance else
                       ("R91_ORIGINAL_189_ONE_FRAME_OVERLAY_RECEIPTS_PASS"
                        if args.render_overlay_acceptance else
                       "R90_ORIGINAL_189_RENDER_DIAGNOSTIC_RECEIPTS_PASS"))))),
                      "pid=" + str(game.pid), "report=" + str(report), flush=True)
                print("R95_SYNTHETIC_OS_INPUT_NOT_PHYSICAL_HARDWARE_KEY_ACCEPTANCE"
                      if args.render_key_acceptance else
                      ("R94_SCRIPTED_OPERATOR_COMMANDS_NOT_PHYSICAL_HOTKEY_ACCEPTANCE"
                       if args.render_manual_acceptance else
                      ("R93_NOT_USER_CONTROLLED_SHOW_HIDE_OR_INTERACTIVE_CLICKGUI"
                       if args.render_visibility_acceptance else
                      ("R92_NOT_INTERACTIVE_CLICKGUI_OR_VISUAL_WINDOW_CERTIFICATION"
                       if args.render_multiframe_acceptance else
                      ("R91_NOT_INTERACTIVE_CLICKGUI_OR_VISUAL_WINDOW_CERTIFICATION"
                       if args.render_overlay_acceptance else
                      "R90_NOT_VISUAL_OR_IN_GAME_CLICKGUI_CERTIFICATION")))), flush=True)
                return
            if args.world_observation:
                print("Open a LOCAL single-player world in the isolated client.", flush=True)
                print("The probe will record counts only; it will not control the game.", flush=True)
                if args.require_ray_entity:
                    print("During the probe, aim the crosshair at an entity at least once.", flush=True)
                input("Once the world is loaded, press Enter to start the read-only observation: ")
                if game.poll() is not None:
                    raise RuntimeError("Minecraft exited before world observation")
            attempts = 1 if args.world_observation else 2
            if args.continuous_observation:
                probe, expected_ticks = "--hook-probe=continuous", None
                markers = tuple(marker.replace("GAME_THREAD_TICK_20_PASS",
                                              "CONTINUOUS_STOP_ON_CLIENT_THREAD_PASS")
                                for marker in MARKERS)
                # The native loader reports the request to the parent console;
                # only the owner-thread completion marker belongs in game stdout.
            elif args.world_observation:
                probe, expected_ticks = "--hook-probe=600", 600
                markers = tuple(marker.replace("GAME_THREAD_TICK_20_PASS",
                                               "GAME_THREAD_TICK_600_PASS")
                                for marker in MARKERS)
            else:
                probe, expected_ticks = "--hook-probe", 20
                markers = MARKERS
            for iteration in range(1, attempts + 1):
                result = subprocess.run(
                    [str(launcher), "attach", str(game.pid), str(jdk), probe],
                    capture_output=True, text=True, timeout=25, env=environment
                )
                if result.returncode != 0 or "ATTACH_RESULT=SUCCESS" not in result.stdout:
                    raise RuntimeError("Attach failed: " + result.stdout + result.stderr)
                if args.continuous_observation:
                    # Only this explicitly authorized manual fixture runs the
                    # owner-thread stop lifecycle. Not in hosted CI.
                    time.sleep(3.0)
                    if game.poll() is not None:
                        raise RuntimeError("Vanilla JVM exited before stop request")
                    stopped = subprocess.run(
                        [str(launcher), "attach", str(game.pid), str(jdk),
                         "--stop-observation"],
                        capture_output=True, text=True, timeout=25, env=environment
                    )
                    if stopped.returncode != 0 or (
                        "mode=stop-observation" not in stopped.stdout
                    ):
                        raise RuntimeError("Stop request failed: " +
                                           stopped.stdout + stopped.stderr)
                for marker in markers:
                    if not wait_for(game, marker, iteration, 55 if args.world_observation else 15):
                        raise RuntimeError("Missing " + marker + " on attach " + str(iteration))
                if "Game crashed!" in read_log() or "NoClassDefFoundError" in read_log():
                    raise RuntimeError("Crash signature detected")
                print("VANILLA_INJECT_PASS", iteration, flush=True)
            evidence, fingerprints = validate_observation_evidence(
                read_log(), attempts, expected_ticks, args.world_observation,
                args.require_ray_entity
            )
            if game.poll() is not None:
                raise RuntimeError("Minecraft exited unexpectedly after cleanup")
            if args.world_observation:
                print("VANILLA_189_CONTINUOUS_STOP_PASS" if args.continuous_observation
                      else "VANILLA_189_WORLD_OBSERVATION_PASS",
                      "worldPlayer=" + evidence[0][1], "rayEntity=" + evidence[0][2],
                      "restoredSha256=" + fingerprints[0], flush=True)
            else:
                print("VANILLA_189_STANDALONE_NATIVE_ACCEPTANCE_PASS",
                      fingerprints[0], flush=True)
        except Exception as exc:
            print("VANILLA_ACCEPTANCE_FAIL", repr(exc), flush=True)
            for line in read_log().splitlines()[-35:]:
                print("VANILLA_LOG", line[-350:], flush=True)
            raise
        finally:
            if game.poll() is None:
                subprocess.run(
                    ["taskkill", "/PID", str(game.pid), "/T", "/F"],
                    capture_output=True, text=True, timeout=15
                )
                try:
                    game.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    game.kill()


if __name__ == "__main__":
    main()
