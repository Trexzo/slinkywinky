#!/usr/bin/env python3
"""R193: prepare a source-guarded, offline-only 66-entry registration correction.

Reads the *pinned* public OpenSlinky source but does not modify it. Produces
only one adjusted module_manager.cpp file at a separate requested path.
No original proprietary assets, DLLs, bytecode or game operations are bundled.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

UPSTREAM_SHA = "fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4"
MANAGER_SHA256 = "d65288ae2c890db7dd09b9783ac5cad7090bd5c32908c07dc3d50061f837de96"
MANAGER_REL = Path("src/engine/jni/module_manager.cpp")
RDATA_REL = Path("src/data/rdata.s")
TABLE_LABEL = "ptr_to_aim_assist_module_descriptor:"
EXTRA_INDEX = 17
UPSTREAM_ENTRIES = 67
ORIGINAL_ENTRIES = 66


def descriptor_entries(source):
    lines = source.splitlines()
    try:
        start = next(i for i, line in enumerate(lines) if line.strip() == TABLE_LABEL)
    except StopIteration as exc:
        raise ValueError("original descriptor-table label not found") from exc
    entries = []
    for line in lines[start + 1:]:
        s = line.strip()
        if s.startswith(".quad "):
            entries.append(s.removeprefix(".quad ").strip())
        else:
            break
    if len(entries) != UPSTREAM_ENTRIES:
        raise ValueError("upstream descriptor table count changed")
    if entries[EXTRA_INDEX] != "chest_stealer_module_descriptor":
        raise ValueError("insertion index/identity changed")
    if len(set(entries)) != len(entries):
        raise ValueError("upstream descriptor table has duplicate pointer symbols")
    return entries


def exactly(source, old, new, count=1):
    if source.count(old) != count:
        raise ValueError("pinned C++ manager expression drifted: " + old[:75])
    return source.replace(old, new)


def patch_manager(source):
    """Pure transformation: compact only the manager's private copy of the table."""
    operations = [
        ("manager->count = 0x43;", "manager->count = 0x42;", 1),
        ("manager->mods = (Module **)libc_calloc(0x43, 8);",
         "manager->mods = (Module **)libc_calloc(0x42, 8);", 1),
        ("libc_memcpy_alt(manager->mods, ptr_to_aim_assist_module_descriptor, 0x218);",
         "libc_memcpy_alt(manager->mods, ptr_to_aim_assist_module_descriptor, 0x88);\n"
         "    libc_memcpy_alt(manager->mods + 17, ptr_to_aim_assist_module_descriptor + 18, 0x188);", 1),
        ("for (unsigned long long i = 0; i != 0x43; i++) {",
         "for (unsigned long long i = 0; i != 0x42; i++) {", 2),
        ("Module *mod = ptr_to_aim_assist_module_descriptor[i];",
         "Module *mod = manager->mods[i];", 2),
        ("manager->en_list = (Module **)libc_calloc(0x43, 8);",
         "manager->en_list = (Module **)libc_calloc(0x42, 8);", 1),
        ("manager->dis_list = (Module **)libc_calloc(0x43, 8);",
         "manager->dis_list = (Module **)libc_calloc(0x42, 8);", 1),
        ("if (*n >= 0x43) return false;",
         "if (*n >= 0x42) return false;", 1),
    ]
    result = source
    for old, new, n in operations:
        result = exactly(result, old, new, n)
    if "ptr_to_aim_assist_module_descriptor[0x43]" not in result:
        raise ValueError("the upstream 67-entry static table declaration unexpectedly changed")
    if 0x88 + 0x188 != ORIGINAL_ENTRIES * 8:
        raise AssertionError("pointer compaction arithmetic changed")
    return result


def make_patch(root, output):
    root = Path(root).resolve(strict=True)
    output = Path(output).resolve()
    if output.is_relative_to(root):
        raise ValueError("refuse to write inside the pinned upstream tree")
    if output.exists():
        raise FileExistsError("refuse to overwrite existing generated file")
    git = subprocess.run(
        ["git", "-C", str(root), "rev-parse", "HEAD"],
        capture_output=True, text=True, check=True, timeout=10,
    )
    observed_sha = git.stdout.strip().lower()
    if observed_sha != UPSTREAM_SHA:
        raise ValueError("pinned upstream commit SHA changed")
    raw = (root / MANAGER_REL).read_bytes()
    source_sha = hashlib.sha256(raw).hexdigest()
    if source_sha != MANAGER_SHA256:
        raise ValueError("upstream manager source changed despite expected commit")
    entries = descriptor_entries((root / RDATA_REL).read_text(encoding="utf-8"))
    updated = patch_manager(raw.decode("utf-8"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as file:
        file.write(updated.encode("utf-8"))
    return {
        "schema": 1, "upstream_sha": observed_sha,
        "manager_source_sha256": source_sha,
        "manager_patched_sha256": hashlib.sha256(updated.encode("utf-8")).hexdigest(),
        "candidate_registration_count": ORIGINAL_ENTRIES,
        "upstream_registration_count": UPSTREAM_ENTRIES,
        "removed_registration": entries[EXTRA_INDEX],
        "removed_index": EXTRA_INDEX,
        "original_static_table_untouched": True,
        "output_file": str(output),
        "native_compiled": False,
        "in_game_validated": False,
        "commercial_parity_verified": False,
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-root", required=True)
    parser.add_argument("--output-file", required=True)
    parser.add_argument("--json-out")
    args = parser.parse_args(argv)
    evidence = make_patch(args.reference_root, args.output_file)
    payload = json.dumps(evidence, indent=2, sort_keys=True) + "\n"
    if args.json_out:
        location = Path(args.json_out).resolve()
        if location.is_relative_to(Path(args.reference_root).resolve()):
            raise ValueError("report must not be written inside original upstream source")
        if location.exists():
            raise FileExistsError("report already exists")
        location.parent.mkdir(parents=True, exist_ok=True)
        with location.open("x", encoding="utf-8") as file:
            file.write(payload)
    print(payload, end="")


if __name__ == "__main__":
    main()
