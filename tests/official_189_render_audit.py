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
        print(result.stdout.rstrip(), flush=True)
        print("R71_OFFLINE_ORIGINAL_JAR_RENDER_PREFLIGHT_PASS", flush=True)


if __name__ == "__main__":
    main()
