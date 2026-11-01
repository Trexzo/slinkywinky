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


def validate_observation_evidence(log_text, attempts, expected_ticks,
                                  require_world=False, require_ray_entity=False):
    """Validate original-process markers; never infer a world from attachment alone."""
    if any(marker in log_text for marker in (
        "GAME_THREAD_HOOK_ABORT", "NATIVE_GAME_THREAD_PROBE_REJECTED",
        "GAME_THREAD_HOOK_CLEANUP_FAIL", "NATIVE_SESSION_CLOSE_REJECTED"
    )):
        raise RuntimeError("Game hook rejection, native close, or restoration failure")
    evidence = re.findall(
        r"NATIVE_OBSERVATION_SUMMARY accepted=(\\d+) worldPlayer=(\\d+) "
        r"rayEntity=(\\d+) generations=(\\d+)", log_text
    )
    if len(evidence) != attempts:
        raise RuntimeError("Expected exactly one accepted-frame summary per attachment")
    for accepted, world_frames, ray_frames, generations in evidence:
        accepted, world_frames, ray_frames, generations = map(
            int, (accepted, world_frames, ray_frames, generations)
        )
        if accepted != expected_ticks:
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
    parser.add_argument("--world-observation", action="store_true",
                        help="Manual-only: enter a local single-player world, then run a 600-tick read-only probe")
    parser.add_argument("--require-ray-entity", action="store_true",
                        help="Also require a native-accepted entity under the crosshair; implies --world-observation")
    args = parser.parse_args()
    if not args.run:
        parser.error("Refusing to start Minecraft: pass --run explicitly")
    if os.name != "nt":
        parser.error("Windows x64 only")
    if args.require_ray_entity and not args.world_observation:
        parser.error("--require-ray-entity requires --world-observation")
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
    log = sandbox / "vanilla-run.log"
    cmd = [
        str(jdk / "bin" / "java.exe"), "-Xmx768M",
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
    environment["SLINKYWINKY_EXPERIMENTAL_HOOK"] = "1"
    if args.world_observation:
        environment["SLINKYWINKY_EXTENDED_DIAGNOSTIC"] = "1"

    def read_log():
        return log.read_text(encoding="utf-8", errors="replace")

    def wait_for(process, marker, count, seconds):
        end = time.monotonic() + seconds
        while time.monotonic() < end and process.poll() is None:
            if read_log().count(marker) >= count:
                return True
            time.sleep(0.25)
        return False

    print("Launching one isolated vanilla 1.8.9 JVM; no Forge.", flush=True)
    with log.open("wb") as output:
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
            if args.world_observation:
                print("Open a LOCAL single-player world in the isolated client.", flush=True)
                print("The probe will record counts only; it will not control the game.", flush=True)
                if args.require_ray_entity:
                    print("During the probe, aim the crosshair at an entity at least once.", flush=True)
                input("Once the world is loaded, press Enter to start the 600-tick observation: ")
                if game.poll() is not None:
                    raise RuntimeError("Minecraft exited before world observation")
            attempts = 1 if args.world_observation else 2
            probe = "--hook-probe=600" if args.world_observation else "--hook-probe"
            expected_ticks = 600 if args.world_observation else 20
            markers = tuple(marker.replace("GAME_THREAD_TICK_20_PASS",
                                           "GAME_THREAD_TICK_600_PASS")
                            for marker in MARKERS)
            if not args.world_observation:
                markers = MARKERS
            for iteration in range(1, attempts + 1):
                result = subprocess.run(
                    [str(launcher), "attach", str(game.pid), str(jdk), probe],
                    capture_output=True, text=True, timeout=25, env=environment
                )
                if result.returncode != 0 or "ATTACH_RESULT=SUCCESS" not in result.stdout:
                    raise RuntimeError("Attach failed: " + result.stdout + result.stderr)
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
                print("VANILLA_189_WORLD_OBSERVATION_PASS",
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
