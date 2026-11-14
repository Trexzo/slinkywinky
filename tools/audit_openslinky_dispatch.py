#!/usr/bin/env python3
"""R172: read-only native event-dispatch comparison for one pinned candidate.

Reads pinned public C++ source and optionally a private decrypted ZIP.
Never loads a native executable, injects anything, or publishes original bytes.
"""
import argparse
import hashlib
import json
import re
import sys
import zipfile
from pathlib import Path

ORIGINAL_TEXT_HASH = "35165183210c4f50cde535dc98b806b29ed7efd8fb3da3822f416915559302af"
CANDIDATE_TEXT_RVA = 0x1000
MAX_TEXT_BYTES = 2 * 1024 * 1024
# SHA-256 of exact decrypted .text instruction windows, including alignment.
# Manually disassembled independently; these hashes identify that one build.
NATIVE_WINDOWS = {
    "dispatch_event": (0x28480, 0x284d0, "639a14cd10b813a425f05084a313ffbec194af1c00d038d3ee3f660be143ae23"),
    "is_event_handled": (0x284d0, 0x284f0, "683f14adaa6a3c19f531ac26331ca4eeab7825845b40526ed4082b39cfe99fbe"),
    "queue_module_toggle": (0x284f0, 0x285b0, "7c00727d076d7e8597ed57d6c5c5690c76bb0c757e7e8704bd3e6b602b7a72e6"),
    "has_queued_modules": (0x285b0, 0x285d0, "71fb521bdf7f5685e8c46ed9410be430cf3706fa030c0037a86d9c685e9ced77"),
    "apply_module_toggles_entry": (0x285d0, 0x28600, "22835c6911abd52e65f96f94e438ecc06bf727aaab5dbbcbb276849ed428c57e"),
}


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def function_body(source, name):
    """Isolate a named C++ function using its signature and balanced braces."""
    regex = re.compile(r'^extern\s+"C"\s+[^;\n{]+\b' + re.escape(name)
                       + r'\s*\([^;{]*\)\s*\{', re.M)
    matches = list(regex.finditer(source))
    if len(matches) != 1:
        raise ValueError(f"Expected one C++ definition of {name}, got {len(matches)}")
    start = matches[0].end() - 1
    depth = 0
    for pos in range(start, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return source[start:pos + 1]
    raise ValueError(f"Unterminated C++ function: {name}")


def source_audit(root):
    manager = (root / "src/engine/jni/module_manager.cpp").read_text(encoding="utf-8")
    dispatch = function_body(manager, "dispatch_event")
    handled = function_body(manager, "is_event_handled")
    queue = function_body(manager, "queue_module_toggle")
    toggles = function_body(manager, "apply_module_toggles")
    ctor = function_body(manager, "create_module_manager")
    if not re.search(r'for\s*\(Handler\s+f\s*=\s*\*h\s*;\s*f\s*;', dispatch):
        raise ValueError("Source lacks expected null-terminated dispatch loop")
    if not re.search(r'manager->lists\s*\[type\]', dispatch):
        raise ValueError("Source dispatch no longer indexes the handler table")
    count = re.search(r'Handler\s*\*lists\s*\[\s*(\d+)\s*\]', manager)
    if not count:
        raise ValueError("Handler table declaration not found")
    n = int(count.group(1))
    if n != 27:
        raise ValueError(f"Unexpected event slot count: {n}")
    guard = r'if\s*\(\s*type\s*>=\s*27\s*\)'
    result = {
        "source_event_slot_count": n,
        "source_dispatch_has_out_of_range_guard": bool(re.search(guard, dispatch)),
        "source_is_handled_has_out_of_range_guard": bool(re.search(guard, handled)),
        "source_toggle_queue_has_capacity_guard": bool(re.search(r'if\s*\(\s*\*n\s*>=\s*0x43\s*\)', queue)),
        "source_toggle_processing_checks_world_and_player": bool(re.search(r'if\s*\(\s*!ctx->world\s*\|\|\s*!ctx->player\s*\)', toggles)),
        "source_manager_registers_67": bool(re.search(r'manager->count\s*=\s*0x43\s*;', ctor)),
        "source_dispatches_null_terminated_handlers": True,
    }
    for name in ("source_dispatch_has_out_of_range_guard",
                 "source_is_handled_has_out_of_range_guard",
                 "source_toggle_queue_has_capacity_guard",
                 "source_toggle_processing_checks_world_and_player",
                 "source_manager_registers_67"):
        if not result[name]:
            raise ValueError(f"Review needed: source safety/fidelity contract changed: {name}")
    return result


def private_native_audit(archive):
    with zipfile.ZipFile(archive) as zf:
        names = [n for n in zf.namelist() if n.endswith("/slinky2.decrypted/text.bin")]
        if len(names) != 1:
            raise ValueError(f"Expected one private decrypted .text member, found {len(names)}")
        info = zf.getinfo(names[0])
        if info.file_size > MAX_TEXT_BYTES:
            raise ValueError(f"Unexpected private decrypted .text size: {info.file_size}")
        original = zf.read(info)
    if sha256(original) != ORIGINAL_TEXT_HASH:
        raise ValueError("Private candidate .text hash mismatches pinned build")
    for name, (start, end, expected_hash) in NATIVE_WINDOWS.items():
        native = original[start - CANDIDATE_TEXT_RVA:end - CANDIDATE_TEXT_RVA]
        if len(native) != end - start or sha256(native) != expected_hash:
            raise ValueError(f"Private native window mismatch: {name}")
    # Observations below derive from independently reviewed disassembly of
    # exactly the five hashed windows, not from an automatic x86 decompiler.
    return {
        "private_candidate_text_sha256": ORIGINAL_TEXT_HASH,
        "private_candidate_function_windows_verified": len(NATIVE_WINDOWS),
        "native_dispatch_event_has_no_event_index_guard": True,
        "native_is_event_handled_has_no_event_index_guard": True,
        "native_queue_toggle_has_no_capacity_guard": True,
        "native_apply_toggles_entry_starts_with_pending_queue_read": True,
        "native_disassembly_method": "reviewed pinned original instruction windows; no code executed",
    }


def audit(source, candidate=None):
    result = {
        "reference": "Skid-Technology/OpenSlinky@fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4",
        "candidate_original_authenticated": False,
        "gameplay_equivalence_verified": False,
        "runtime_or_injector_executed": False,
    }
    result.update(source_audit(source))
    if candidate is None:
        result["private_candidate_function_windows_verified"] = None
        result["note"] = "Source-only audit; native instruction checks need private candidate ZIP."
    else:
        result.update(private_native_audit(candidate))
        result["event_type_guard_is_source_only"] = result["source_dispatch_has_out_of_range_guard"]
        result["toggle_queue_capacity_guard_is_source_only"] = result["source_toggle_queue_has_capacity_guard"]
        result["note"] = "Preserve safety guards; differences are not evidence of gameplay parity."
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source", type=Path, default=Path("references/OpenSlinky"))
    p.add_argument("--candidate-zip", type=Path, help="Optional private local ZIP; never upload")
    p.add_argument("--json-out", type=Path, help="Metadata-only JSON, no native bytes")
    args = p.parse_args()
    try:
        result = audit(args.source, args.candidate_zip)
        if args.json_out:
            args.json_out.parent.mkdir(parents=True, exist_ok=True)
            args.json_out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    except (OSError, ValueError, zipfile.BadZipFile) as exc:
        print(f"R172 source/native dispatch audit failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
