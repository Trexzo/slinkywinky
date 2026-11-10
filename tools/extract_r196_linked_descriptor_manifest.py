#!/usr/bin/env python3
"""R196: inspect *compiled* native descriptor metadata without loading the DLL.

Uses the linked PE data and map, never upstream code execution. Emits category,
enabled-default, event IDs and signed priorities only; never writes symbol
addresses, original strings, native byte sequences or third-party source.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from prepare_openslinky_registration_66 import RDATA_REL, RDATA_SHA256, descriptor_entries
from verify_compiled_registration_66 import (
    TABLE_SYMBOL, audit_linked_pointer_table, map_rva_to_offset,
    parse_map_symbols,
)

SLOT_COUNT = 27
ORIGINAL_COUNT = 66
UPSTREAM_COUNT = 67
OMITTED_INDEX = 17
MODULE_SIZE = 0x48 + SLOT_COUNT * 16


def extract_descriptor_metadata(dll_bytes, address, image_base):
    if address < image_base:
        raise ValueError("descriptor address below image base")
    _, offset, section = map_rva_to_offset(dll_bytes, address - image_base, MODULE_SIZE)
    if section != ".data":
        raise ValueError("descriptor not fully inside .data")
    cat, = struct.unpack_from("<I", dll_bytes, offset + 0x10)
    enabled = dll_bytes[offset + 0x14]
    if cat >= 5 or enabled not in (0, 1):
        raise ValueError("invalid linked descriptor category or initial enabled flag")
    handlers = []
    for event_id in range(SLOT_COUNT):
        slot = offset + 0x48 + event_id * 16
        callback, = struct.unpack_from("<Q", dll_bytes, slot)
        priority, = struct.unpack_from("<i", dll_bytes, slot + 8)
        if callback == 0:
            # Only a nonzero callback constitutes an event registration.
            continue
        if not image_base <= callback < image_base + 0x400000:
            raise ValueError("linked native callback outside expected image")
        # Offline harness only handles small normal signed priorities.
        if not -100000 <= priority <= 100000:
            raise ValueError("callback priority outside expected integer range")
        handlers.append({"event": event_id, "priority": priority})
    return {"category": cat, "default_enabled": bool(enabled), "handlers": handlers}


def inspect(upstream_root, dll_path, map_path):
    upstream_root = Path(upstream_root)
    source_bytes = (upstream_root / RDATA_REL).read_bytes()
    if hashlib.sha256(source_bytes).hexdigest() != RDATA_SHA256:
        raise ValueError("upstream rdata source hash mismatch")
    names = descriptor_entries(source_bytes.decode("utf-8"))
    if len(names) != UPSTREAM_COUNT or names[OMITTED_INDEX] != "chest_stealer_module_descriptor":
        raise ValueError("unexpected pinned upstream descriptor layout")
    audit_linked_pointer_table(dll_path, map_path, names)
    dll_bytes = Path(dll_path).read_bytes()
    maps = parse_map_symbols(Path(map_path).read_text(encoding="utf-8"), names)
    nt = struct.unpack_from("<I", dll_bytes, 0x3C)[0]
    image_base, = struct.unpack_from("<Q", dll_bytes, nt + 24 + 24)
    active = []
    per_event = [0] * SLOT_COUNT
    per_cat = [0] * 5
    for original_index, name in enumerate(names):
        if original_index == OMITTED_INDEX:
            continue
        meta = extract_descriptor_metadata(dll_bytes, maps[name], image_base)
        per_cat[meta["category"]] += 1
        for handler in meta["handlers"]:
            per_event[handler["event"]] += 1
        active.append({"source_index": original_index, **meta})
    if len(active) != ORIGINAL_COUNT:
        raise ValueError("wrong compacted metadata count")
    if sum(per_event) != 171:
        raise ValueError("linked callback metadata does not match pinned 171 original subscriptions")
    if active[17]["source_index"] != 18:
        raise ValueError("failed to compact private registration sequence")
    return {
        "schema": 1, "kind": "sanitized-linked-native-descriptor-metadata",
        "dll_sha256": hashlib.sha256(dll_bytes).hexdigest(),
        "static_descriptor_count": UPSTREAM_COUNT,
        "active_registration_count": ORIGINAL_COUNT,
        "omitted_source_index": OMITTED_INDEX,
        "callback_subscriptions": sum(per_event),
        "category_counts": per_cat, "event_counts": per_event,
        "entries": active,
        "contains_raw_callback_addresses": False,
        "native_code_executed": False, "runtime_parity_verified": False,
    }


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--upstream", required=True)
    p.add_argument("--dll", required=True)
    p.add_argument("--map", required=True)
    p.add_argument("--json-out")
    args = p.parse_args(argv)
    result = inspect(args.upstream, args.dll, args.map)
    text = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.json_out:
        outfile = Path(args.json_out)
        if outfile.exists():
            raise FileExistsError("refusing to overwrite existing descriptor evidence")
        outfile.parent.mkdir(parents=True, exist_ok=True)
        outfile.write_text(text, encoding="utf-8")
    print(json.dumps({key: value for key, value in result.items() if key != "entries"}, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
