#!/usr/bin/env python3
"""R71: source-verified, ephemeral vanilla 1.8.9 renderer audit.

This downloads the *original* Mojang client to the CI runner's temporary
directory, validates length + SHA-1, runs the existing read-only Java8 audit,
and removes the JAR. The JAR is never added to the repository or an artifact.
"""
import argparse
import hashlib
import json
import os
import pathlib
import subprocess
import sys
import tempfile
import urllib.parse
import urllib.request

SHA1 = "3870888a6c3d349d3771a3e9d16c9bf5e076b908"
SIZE = 8461484
VERSION = "1.8.9"
# Exact classfile digests independently measured by R71 GitHub Actions
# from the SHA-1-verified original Mojang client. Never use these as proof
# that a modified third-party launcher or a running JVM is equivalent.
EXPECTED_CLASSES = {
    "R71_ORIGINAL_MINECRAFT_CLASS_SHA256":
        "430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82",
    "R71_ORIGINAL_RENDERER_CLASS_SHA256":
        "8a4ce93bbf272398ea298d2ed363f75211dd469fcd3b66682a387e4738a419a2",
}
EXPECTED_CALLSITE = "caller=ave.av renderer=bfk method=a descriptor=(FJ)V"
MANIFEST = (
    "https://piston-meta.mojang.com/v1/packages/"
    "d546f1707a3f2b7d034eece5ea2e311eda875787/1.8.9.json"
)
OFFICIAL_ORIGINS = {"launcher.mojang.com", "piston-data.mojang.com"}


def permitted_download(url):
    parsed = urllib.parse.urlparse(url)
    return parsed.scheme == "https" and parsed.hostname in OFFICIAL_ORIGINS and (
        parsed.username is None and parsed.password is None and parsed.port is None
    ) and parsed.path.endswith("/" + SHA1 + "/client.jar")


def read_metadata(opener=urllib.request.urlopen):
    with opener(MANIFEST, timeout=30) as response:
        # Metadata should be tiny; refuse unexpectedly large responses.
        raw = response.read(256000 + 1)
    if len(raw) > 256000:
        raise ValueError("Oversized official version metadata")
    metadata = json.loads(raw.decode("utf-8"))
    client = metadata.get("downloads", {}).get("client", {})
    if (metadata.get("id") != VERSION
            or client.get("sha1") != SHA1
            or client.get("size") != SIZE
            or not permitted_download(client.get("url", ""))):
        raise ValueError("Original Mojang 1.8.9 metadata admission failed")
    return client["url"]


def fetch_exact(url, destination, opener=urllib.request.urlopen):
    if not permitted_download(url):
        raise ValueError("Non-Mojang client URL refused")
    h = hashlib.sha1()
    total = 0
    # Open with an explicit user-agent and enforce the limit regardless of
    # Content-Length. The final path must be in an isolated temporary folder.
    request = urllib.request.Request(url, headers={"User-Agent": "SlinkyWinky-Original189Audit/1"})
    with opener(request, timeout=45) as response:
        final_url = response.geturl()
        if not permitted_download(final_url):
            raise ValueError("Unexpected redirect outside Mojang client origin")
        with open(destination, "xb") as target:
            while True:
                chunk = response.read(65536)
                if not chunk:
                    break
                total += len(chunk)
                if total > SIZE:
                    raise ValueError("Client exceeds official declared size")
                target.write(chunk)
                h.update(chunk)
    if total != SIZE or h.hexdigest() != SHA1:
        raise ValueError("Mojang original client SHA-1 or size mismatch")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--java8", required=True)
    parser.add_argument("--audit-jar", required=True)
    parser.add_argument("--instrument-jar", required=True)
    args = parser.parse_args()
    java = pathlib.Path(args.java8).resolve(strict=True)
    audit = pathlib.Path(args.audit_jar).resolve(strict=True)
    instrument = pathlib.Path(args.instrument_jar).resolve(strict=True)
    if not (java.is_file() and audit.is_file() and instrument.is_file()):
        raise ValueError("Missing Java 8 or compiled local verifier")
    # Always delete the original client bytes on success, audit failure, or
    # network failure. The only output is the method-candidate diagnostic.
    with tempfile.TemporaryDirectory(prefix="slinkywinky-original189-") as root:
        destination = pathlib.Path(root, "client.jar")
        source_url = read_metadata()
        alternatives = [source_url, (
            "https://piston-data.mojang.com/v1/objects/" + SHA1 + "/client.jar"
        )]
        last_error = None
        for url in dict.fromkeys(alternatives):
            try:
                destination.unlink(missing_ok=True)
                fetch_exact(url, destination)
                last_error = None
                break
            except (OSError, ValueError, TimeoutError) as problem:
                last_error = problem
                destination.unlink(missing_ok=True)
        if last_error is not None:
            raise RuntimeError("Both official Mojang download endpoints failed") from last_error
        print("R71_ORIGINAL_MOJANG_SHA1_AND_SIZE_PASS", flush=True)
        cp = str(audit) + os.pathsep + str(instrument)
        result = subprocess.run(
            [str(java), "-cp", cp,
             "dev.slinkywinky.attach.RenderCallsiteAudit",
             str(destination.resolve())],
            check=False, capture_output=True, text=True, timeout=40
        )
        if result.returncode != 0:
            raise RuntimeError("R64 structural renderer audit did not admit a unique method: " +
                               result.stderr[-1200:])
        if (result.stdout.count("R64_RENDER_CALLSITE_CANDIDATE ") != 1
                or "R64_ORIGINAL_JAR_SHA1_VALIDATED" not in result.stdout):
            raise RuntimeError("R64 renderer audit evidence incomplete")
        for label, expected in EXPECTED_CLASSES.items():
            if result.stdout.count(label + "=" + expected + "\n") != 1:
                raise RuntimeError("Original class digest mismatch: " + label)
        if result.stdout.count(
                "R64_RENDER_CALLSITE_CANDIDATE " + EXPECTED_CALLSITE + "\n"
        ) != 1:
            raise RuntimeError("Original 1.8.9 renderer fingerprint changed")
        print(result.stdout.rstrip(), flush=True)
        print("R71_OFFLINE_ORIGINAL_JAR_RENDER_PREFLIGHT_PASS", flush=True)
        # R107: source-pin the NORMAL Minecraft keyboard dispatch owner.
        # This is an offline audit only, NOT a modal/no-pass-through test.
        keyboard = subprocess.run(
            [str(java), "-cp", cp,
             "dev.slinkywinky.attach.KeyboardInputCallsiteAudit",
             str(destination.resolve())],
            check=False, capture_output=True, text=True, timeout=40)
        if (keyboard.returncode != 0 or
                keyboard.stdout.count("R107_ORIGINAL_189_KEYBOARD_EVENT_PATH_AUDITED ") != 1 or
                "owner=ave.s()V next=1 eventKey=2 eventState=3 eventChar=1" not in keyboard.stdout or
                "R107_NORMAL_GAMEPLAY_DISPATCH_PRESENT_NOT_MODAL_INPUT_CERTIFICATION"
                not in keyboard.stdout):
            raise RuntimeError("Original Minecraft keyboard event audit failed: " +
                               keyboard.stdout[-1600:] + keyboard.stderr[-1600:])
        print(keyboard.stdout.rstrip(), flush=True)
        # R109: verify exactly one source-pinned Keyboard.next() callsite
        # can be replaced in memory, keeping all other instructions unchanged.
        # The marker is intentionally unresolved and is NEVER executed.
        key_weave = subprocess.run(
            [str(java), "-cp", cp,
             "dev.slinkywinky.attach.OriginalKeyboardLoopWeaveAudit",
             str(destination.resolve())],
            check=False, capture_output=True, text=True, timeout=40)
        if (key_weave.returncode != 0 or
                key_weave.stdout.count(
                    "R109_ORIGINAL_189_KEYBOARD_NEXT_EXACT_ONE_CALLSITE_MEMORY_ONLY_PASS") != 1 or
                "R109_ALL_OTHER_METHOD_AND_INSTRUCTION_TRACES_UNCHANGED_PASS"
                not in key_weave.stdout or
                "R109_NO_GAME_PROCESS_NO_CLASS_REDEFINITION_NO_INPUT_CAPTURE"
                not in key_weave.stdout):
            raise RuntimeError("R109 original keyboard loop dry run failed: " +
                               key_weave.stdout[-1600:] + key_weave.stderr[-1600:])
        print(key_weave.stdout.rstrip(), flush=True)
        # R111: production-compiled Java8 bridge is still dormant; verify
        # one original Keyboard.next() substitution against Mojang bytes.
        bridge = subprocess.run(
            [str(java), "-cp", cp,
             "dev.slinkywinky.attach.OriginalKeyboardBridgeWeaveAudit",
             str(destination.resolve())],
            check=False, capture_output=True, text=True, timeout=40)
        if (bridge.returncode != 0 or
                "R111_ORIGINAL_189_PRODUCTION_BRIDGE_ONE_CALLSITE_OFFLINE_PASS"
                not in bridge.stdout or
                "R111_ALL_OTHER_METHOD_AND_INSTRUCTION_TRACES_UNCHANGED_PASS"
                not in bridge.stdout or
                "R111_PREQUEUE_KEYBINDINGS_REMAIN_ACTIVE_NOT_MODAL_CERTIFICATION"
                not in bridge.stdout):
            raise RuntimeError("R111 pinned original keyboard bridge failed: " +
                               bridge.stdout[-1600:] + bridge.stderr[-1600:])
        print(bridge.stdout.rstrip(), flush=True)
        # R162: SHA-pinned original ave.s()V source-order audit.
        # The pre-existing JAR has already passed the official SHA-1 check;
        # this invokes the separate test-only ASM inspector without game attach.
        ordered = subprocess.run(
            [str(java), "-cp", cp,
             "dev.slinkywinky.attach.R162InputOrderLedgerSmoke",
             str(destination.resolve())],
            check=False, capture_output=True, text=True, timeout=40)
        if (ordered.returncode != 0 or
                "R162_ORIGINAL_COUNT_PRESERVING_MUTATION_DETECTED_PASS"
                not in ordered.stdout or
                "R162_QUEUE_ONLY_IN_MEMORY_WEAVE_ORDER_UNCHANGED_PASS"
                not in ordered.stdout or
                "R162_ORIGINAL_AVE_INPUT_CALLSITE_SEQUENCE_SHA256 "
                "6b58cf399b237ce7a84a84f1a62a8f5b56dc10e457f6217ea35aaadf99403e7f"
                not in ordered.stdout):
            raise RuntimeError("Original 1.8.9 R162 input-order audit failed: " +
                               ordered.stdout[-1600:] + ordered.stderr[-1600:])
        print(ordered.stdout.rstrip(), flush=True)
        # R163: independent branch/exception graph, original SHA pin and
        # original-call-order-preserving branch mutation negative control.
        graph = subprocess.run(
            [str(java), "-cp", cp,
             "dev.slinkywinky.attach.R163InputControlGraphSmoke",
             str(destination.resolve())],
            check=False, capture_output=True, text=True, timeout=40)
        if (graph.returncode != 0 or
                "R163_ORIGINAL_COUNT_AND_ORDER_PRESERVING_BRANCH_MUTATION_DETECTED"
                not in graph.stdout or
                "R163_QUEUE_ONLY_MEMORY_WEAVE_CFG_IDENTICAL"
                not in graph.stdout or
                "R163_ORIGINAL_VANILLA_INPUT_CFG_SHA256 "
                "2b9173842b6b507e45190da633e9c83d5f9d0e5d2969ff181995b8970fd97256"
                not in graph.stdout):
            raise RuntimeError("R163 Mojang original input CFG mismatch: " +
                               graph.stdout[-1800:] + graph.stderr[-1800:])
        print(graph.stdout.rstrip(), flush=True)
        # R164: test-only whole-input-method entry gate. Original Mojang
        # bytes remain unchanged, no live game/class is transformed.
        cut = subprocess.run(
            [str(java), "-cp", cp,
             "dev.slinkywinky.attach.R164OfflineInputCutSmoke",
             str(destination.resolve())],
            check=False, capture_output=True, text=True, timeout=45)
        if (cut.returncode != 0 or
                "R164_ORIGINAL_AVE_ENTRY_ONLY_MODAL_CUT_SHAPE_PASS"
                not in cut.stdout or
                "R164_ORIGINAL_58_INPUT_PATHS_UNCHANGED_VANILLA_BRANCH_PASS"
                not in cut.stdout or
                "R164_ORIGINAL_REVERSED_GATE_AND_MISSING_RETURN_REFUSED_PASS"
                not in cut.stdout or
                "R164_SOURCE_IN_MEMORY_COPY_RESTORED_OFFLINE_ONLY_PASS"
                not in cut.stdout):
            raise RuntimeError("R164 original Minecraft input-cut probe failed: " +
                               cut.stdout[-1700:] + cut.stderr[-1700:])
        print(cut.stdout.rstrip(), flush=True)
        # R171: independently scan *every* exact original class for LWJGL2
        # and keybind consumers OUTSIDE ave.s()V. Source-only; no Attach.
        global_input = subprocess.run(
            [str(java), "-cp", cp,
             "dev.slinkywinky.attach.R171VanillaJarInputSurfaceSmoke",
             str(destination.resolve())],
            check=False, capture_output=True, text=True, timeout=70)
        if (global_input.returncode != 0 or
                "R171_PINNED_VANILLA_189_GLOBAL_2507_CLASSES_PASS"
                not in global_input.stdout or
                "R171_PINNED_VANILLA_189_GLOBAL_SURFACE_SHA256 "
                "b029538b90e0e23947c9224779028679f0faced032f5a469b07f69b8859dd11e"
                not in global_input.stdout or
                "R171_ORIGINAL_RENDERER_EXTERNAL_INPUT_MUTATION_DETECTED"
                not in global_input.stdout or
                "R171_AVE_METHOD_STATIC_AUDIT_UNCHANGED_BUT_UNSAFE"
                not in global_input.stdout):
            raise RuntimeError("R171 original Minecraft global input audit failed: " +
                               global_input.stdout[-1900:] +
                               global_input.stderr[-1900:])
        print(global_input.stdout.rstrip(), flush=True)
        # R194: follow only exact declared method call targets upstream of
        # R171 input sites across the pinned original Java 8 class corpus.
        # Deliberately fail to certify virtual/interface/reflective coverage.
        call_graph = subprocess.run(
            [str(java), "-cp", cp,
             "dev.slinkywinky.attach.R194OriginalInputCallReachabilitySmoke",
             str(destination.resolve())],
            check=False, capture_output=True, text=True, timeout=70)
        if (call_graph.returncode != 0 or
                "R194_EXACT_PINNED_JAR_GRAPH_PASS"
                not in call_graph.stdout or
                "R194_VIRTUAL_DISPATCH_UNRESOLVED_NEGATIVE_CONTROL_PASS"
                not in call_graph.stdout or
                "R194_GRAPH_SHA256 "
                "f0b1cb8aee451b56302a0035ec92665829c774a251718b4e00f61852d9e3bdfd"
                not in call_graph.stdout or
                "R194_NOT_VIRTUAL_OR_HARDWARE_DISPATCH_PROOF"
                not in call_graph.stdout):
            raise RuntimeError("R194 original 1.8.9 declared-call reachability failed: "+
                               call_graph.stdout[-2200:]+call_graph.stderr[-2200:])
        print(call_graph.stdout.rstrip(), flush=True)
        # R173: exact-source GUI singleton getter and axu screen field,
        # plus same-count getter mutation negative control. No game attach.
        original_gui = subprocess.run(
            [str(java), "-cp", cp,
             "dev.slinkywinky.attach.R173OriginalGuiFieldAudit",
             str(destination.resolve())],
            check=False, capture_output=True, text=True, timeout=70)
        if (original_gui.returncode != 0 or
                "R173_MOJANG_189_AVE_SINGLETON_GUI_FIELD_EXACT_SOURCE_PASS"
                not in original_gui.stdout or
                "R173_ALTERNATE_SINGLETON_NEGATIVE_CONTROL_PASS"
                not in original_gui.stdout or
                "R173_OFFLINE_ONLY_ORIGINAL_BYTES_UNCHANGED"
                not in original_gui.stdout):
            raise RuntimeError("R173 exact Mojang GUI field audit failed: " +
                               original_gui.stdout[-1400:] +
                               original_gui.stderr[-1400:])
        print(original_gui.stdout.rstrip(), flush=True)

        # R72: a separate, offline-only ASM dry run of the SHA-pinned bfk
        # method. The Java helper never writes modified game class bytes.
        weave = subprocess.run(
            [str(java), "-cp", cp,
             "dev.slinkywinky.attach.OriginalRendererWeaveAudit",
             str(destination.resolve())],
            check=False, capture_output=True, text=True, timeout=40
        )
        if (weave.returncode != 0 or
                weave.stdout.count("R72_ORIGINAL_BFK_OFFLINE_WEAVE_PREFLIGHT_PASS") != 1 or
                weave.stdout.count("R72_NO_GAME_ATTACH_OR_TRANSFORMED_BINARY_OUTPUT") != 1):
            raise RuntimeError("R72 original-class memory-only weave was not certified: " +
                               weave.stdout[-1600:] + weave.stderr[-1600:])
        print("R72_ORIGINAL_RENDERER_OFFLINE_WEAVE_PASS", flush=True)
        # R73: verify that the SHA-gated, strict loaded-class admission
        # policy accepts exactly the original Mojang ave/bfk class bytes.
        # No Java Attach, class loading, Minecraft process, or graphics used.
        static_admission = subprocess.run(
            [str(java), "-cp", cp,
             "dev.slinkywinky.attach.RendererLivePreflightExactAudit",
             str(destination.resolve())],
            check=False, capture_output=True, text=True, timeout=40
        )
        if (static_admission.returncode != 0 or
                static_admission.stdout.count(
                    "R73_STATIC_ORIGINAL_RENDERER_ADMISSION_PASS") != 1 or
                static_admission.stdout.count(
                    "R73_STATIC_AUDIT_IS_NOT_LIVE_GAME_EVIDENCE") != 1):
            raise RuntimeError("R73 original class preflight policy mismatch: " +
                               static_admission.stdout[-1600:] +
                               static_admission.stderr[-1600:])
        print("R73_STATIC_ORIGINAL_RENDERER_PREFLIGHT_PASS", flush=True)
        # R75: certify the actual (not marker-only) production callback prefix,
        # against the exact Mojang class, entirely in memory. Do not attach.
        real_prefix = subprocess.run(
            [str(java), "-cp", cp,
             "dev.slinkywinky.attach.OriginalRendererFrameDiagnosticAudit",
             str(destination.resolve())],
            check=False, capture_output=True, text=True, timeout=40
        )
        if (real_prefix.returncode != 0 or
                real_prefix.stdout.count(
                    "R75_PINNED_ORIGINAL_RENDER_CALLBACK_PREFIX_PASS") != 1 or
                real_prefix.stdout.count(
                    "R75_MEMORY_ONLY_NO_GAME_ATTACH_PASS") != 1):
            raise RuntimeError("R75 pinned production callback prefix mismatch: " +
                               real_prefix.stdout[-1800:] +
                               real_prefix.stderr[-1800:])
        print("R75_ORIGINAL_RENDERER_CALLBACK_PREFIX_OFFLINE_PASS", flush=True)


if __name__ == "__main__":
    main()
