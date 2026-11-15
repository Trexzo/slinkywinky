#!/usr/bin/env python3
"""Read-only R192 preflight for the pinned upstream OpenSlinky native build.

Does not compile, execute or redistribute third-party source or DLLs.
Non-strict mode is intentionally informational: no LLVM on CI is not a test pass.
"""
import argparse
import json
from pathlib import Path
import shutil
import subprocess

PINNED_SHA = "fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4"
REQUIRED_TOOLS = (
    "cmake", "ninja", "clang", "clang-cl", "lld-link",
    "llvm-rc", "llvm-mt", "llvm-lib", "llvm-dlltool",
)
REQUIRED_FILES = (
    "CMakeLists.txt",
    "cmake/toolchain-windows-clang-cl.cmake",
    "src/data/data.s", "src/data/rdata.s", "src/data/iat.s",
    "src/platform/win/chkstk.s",
)

def probe(root, which=shutil.which, git_runner=None):
    """Build a metadata-only report; unknown provenance always fails closed."""
    root = Path(root).resolve()
    found = {name: which(name) for name in REQUIRED_TOOLS}
    missing_tools = [name for name, path in found.items() if not path]
    missing_files = [name for name in REQUIRED_FILES if not (root / name).is_file()]
    runner = git_runner or subprocess.run
    sha = None
    if root.is_dir() and not missing_files:
        try:
            result = runner(
                ["git", "-C", str(root), "rev-parse", "HEAD"],
                capture_output=True, text=True, timeout=10, check=False,
            )
            if result.returncode == 0:
                sha = result.stdout.strip().lower()
        except (OSError, subprocess.TimeoutExpired):
            pass
    pinned = sha == PINNED_SHA
    ready = not missing_tools and not missing_files and pinned
    return {
        "schema": 1,
        "reference": "Skid-Technology/OpenSlinky",
        "expected_sha": PINNED_SHA,
        "observed_sha": sha,
        "pinned_source_verified": pinned,
        "source_layout_verified": not missing_files,
        "missing_source_files": missing_files,
        "missing_tools": missing_tools,
        "tool_paths": found,
        "toolchain_ready": ready,
        "dll_compiled": False,
        "dll_executed": False,
        "minecraft_tested": False,
        "fidelity_verified": False,
    }

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1] / "references" / "OpenSlinky")
    parser.add_argument("--json-out", type=Path)
    parser.add_argument("--strict", action="store_true", help="fail when the native build prerequisites are unavailable")
    args = parser.parse_args(argv)
    report = probe(args.root)
    payload = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.json_out:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.write_text(payload, encoding="utf-8")
    print(payload, end="")
    return 0 if report["toolchain_ready"] or not args.strict else 1

if __name__ == "__main__":
    raise SystemExit(main())
