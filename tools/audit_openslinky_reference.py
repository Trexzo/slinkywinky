#!/usr/bin/env python3
"""Read-only source inventory for a pinned, external OpenSlinky reference.

Never builds, executes, injects or redistributes the external native client.
"""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

PIN = "fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4"
EXPECTED_CPP_MODULES = 66
EXPECTED_FUNCTION_MAP = 1338
EXPECTED_GLOBAL_MAP = 7788
DECRYPTED_CANDIDATE_REGISTERED = 66


def audit(reference: Path, matrix: Path):
    reference = reference.resolve()
    manager_file = reference / "src/engine/jni/module_manager.cpp"
    symbols_file = reference / "docs/symbol-map.json"
    global_file = reference / "docs/global-symbol-map.json"
    for file in (manager_file, symbols_file, global_file):
        if not file.is_file():
            raise ValueError(f"Missing reference source: {file}")
    commit = None
    head = subprocess.run(
        ["git", "-C", str(reference), "rev-parse", "HEAD"],
        capture_output=True, text=True, check=False,
    )
    if head.returncode == 0:
        commit = head.stdout.strip()
        if commit != PIN:
            raise ValueError(f"Reference commit changed: {commit}; expected {PIN}")
    cpp = sorted((reference / "src/logic").glob("**/*.cpp"))
    symbols = json.loads(symbols_file.read_text(encoding="utf-8"))
    globals_map = json.loads(global_file.read_text(encoding="utf-8"))
    text = manager_file.read_text(encoding="utf-8")
    count_match = re.search(r"manager->count\s*=\s*(0x[0-9A-Fa-f]+|[0-9]+)", text)
    if count_match is None:
        raise ValueError("Cannot locate the manager registration count")
    count = int(count_match.group(1), 0)
    catalog = set()
    if matrix.is_file():
        regex = r"^\|\s*(?:block|combat|move|utility|visual)\s*\|\s*`([^`]+)`"
        catalog = set(re.findall(regex, matrix.read_text(encoding="utf-8"), re.MULTILINE))
    source_names = {p.stem for p in cpp}
    result = {
        "upstream_repository": "Skid-Technology/OpenSlinky",
        "expected_upstream_commit": PIN,
        "checked_out_commit": commit,
        "cpp_logic_module_files": len(cpp),
        "function_symbol_mappings": len(symbols),
        "global_symbol_mappings": len(globals_map),
        "candidate_decrypted_registered_modules": DECRYPTED_CANDIDATE_REGISTERED,
        "new_cpp_registered_modules": count,
        "registration_delta": count - DECRYPTED_CANDIDATE_REGISTERED,
        "module_logic_missing_from_local_matrix": sorted(source_names - catalog) if catalog else None,
        "local_matrix_entries_without_cpp_file": sorted(catalog - source_names) if catalog else None,
        "source_build_or_runtime_tested": False,
        "official_original_provenance_authenticated": False,
    }
    if len(cpp) != EXPECTED_CPP_MODULES:
        raise ValueError(f"Module count drift: {len(cpp)} != {EXPECTED_CPP_MODULES}")
    if len(symbols) != EXPECTED_FUNCTION_MAP or len(globals_map) != EXPECTED_GLOBAL_MAP:
        raise ValueError("Native symbol map sizes changed")
    if count not in (66, 67):
        raise ValueError(f"Unreviewed module registration count: {count}")
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--reference", type=Path, default=Path("references/OpenSlinky"))
    p.add_argument("--matrix", type=Path, default=Path("docs/MODULE_MATRIX.md"))
    p.add_argument("--json-out", type=Path)
    p.add_argument("--strict-native-count", action="store_true",
                   help="Fail if the C++ registration count differs from the candidate decrypted DLL")
    args = p.parse_args()
    try:
        result = audit(args.reference, args.matrix)
        if args.json_out:
            args.json_out.parent.mkdir(parents=True, exist_ok=True)
            args.json_out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(result, indent=2))
        if result["registration_delta"]:
            print("KNOWN FIDELITY GAP: C++ registration differs from candidate DLL", file=sys.stderr)
        if args.strict_native_count and result["registration_delta"]:
            return 2
        return 0
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"Reference audit failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
