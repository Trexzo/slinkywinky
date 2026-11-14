#!/usr/bin/env python3
"""R197: private, hash-pinned native PE descriptor-record export.

Preserves exact 0x1f8-byte compiled .data records, INCLUDING unsafe native
pointers. Output must remain in an isolated private directory and must never
be committed/shared. The C++ test harness sanitizes all callable pointers
before entering the linked native manager. This exporter does not execute code.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from extract_r196_linked_descriptor_manifest import inspect, MODULE_SIZE
from prepare_openslinky_registration_66 import RDATA_REL, descriptor_entries
from verify_compiled_registration_66 import map_rva_to_offset, parse_map_symbols

EXPECTED_DLL_SHA256 = "3613a466433fb1fa83d2d9e96815ece719c166f881c78642dc18d3e13ae2aa14"
MAGIC = b"R197REC1"
HEADER = struct.Struct("<8sII32s")
ENTRY = struct.Struct("<I")
OMITTED_INDEX = 17
RECORD_COUNT = 66
SOURCE_COUNT = 67


def serialize_records(records, dll_hash):
    """Pure serializer, safe for synthetic fixtures (no addresses here)."""
    if len(records) != RECORD_COUNT:
        raise ValueError("record count differs from pinned 66")
    if len(dll_hash) != 32:
        raise ValueError("expected exactly 32 digest bytes")
    output = bytearray(HEADER.pack(MAGIC, RECORD_COUNT, MODULE_SIZE, dll_hash))
    for compact, (source, payload) in enumerate(records):
        expected_index = compact if compact < OMITTED_INDEX else compact + 1
        if source != expected_index:
            raise ValueError("native source descriptor order/index changed")
        if len(payload) != MODULE_SIZE:
            raise ValueError("native module record must have exact 0x1f8 bytes")
        output.extend(ENTRY.pack(source))
        output.extend(payload)
    return bytes(output)


def export_records(upstream_root, dll_path, map_path, outfile):
    upstream_root = Path(upstream_root).resolve(strict=True)
    dll_path = Path(dll_path).resolve(strict=True)
    map_path = Path(map_path).resolve(strict=True)
    outfile = Path(outfile).resolve()
    repo_root = Path(__file__).resolve().parent.parent
    if outfile.is_relative_to(repo_root) or outfile.is_relative_to(upstream_root):
        raise ValueError("refusing to export native pointer-bearing records into any source checkout")
    if outfile.exists():
        raise FileExistsError("refusing to overwrite private descriptor fixture")

    # Reuse the pinned-source SHA, module-symbol PE pointer-table and 171
    # callback-subscription checks before exposing any record bytes.
    audit = inspect(upstream_root, dll_path, map_path)
    if audit["dll_sha256"] != EXPECTED_DLL_SHA256:
        raise ValueError("unexpected corrected DLL fingerprint")
    dll_bytes = dll_path.read_bytes()
    names = descriptor_entries((upstream_root / RDATA_REL).read_text(encoding="utf-8"))
    required = parse_map_symbols(map_path.read_text(encoding="utf-8"), names)
    nt = struct.unpack_from("<I", dll_bytes, 0x3c)[0]
    image_base, = struct.unpack_from("<Q", dll_bytes, nt + 24 + 24)
    records = []
    for i, item in enumerate(audit["entries"]):
        source_index = i if i < OMITTED_INDEX else i + 1
        if item["source_index"] != source_index:
            raise ValueError("sanitized linked descriptor/source index drift")
        addr = required[names[source_index]]
        if addr < image_base:
            raise ValueError("native descriptor below linked image base")
        _, offset, section = map_rva_to_offset(dll_bytes, addr - image_base, MODULE_SIZE)
        if section != ".data":
            raise ValueError("native descriptor record not in .data")
        record = dll_bytes[offset:offset + MODULE_SIZE]
        # Independently re-check the field offsets copied into this record.
        cat, = struct.unpack_from("<I", record, 0x10)
        enabled = record[0x14]
        if cat != item["category"] or enabled != int(item["default_enabled"]):
            raise ValueError("copied raw record category or enabled flag disagrees with metadata")
        for event in range(27):
            handler = next((h for h in item["handlers"] if h["event"] == event), None)
            fn, = struct.unpack_from("<Q", record, 0x48 + event * 16)
            if (fn != 0) != (handler is not None):
                raise ValueError("copied raw callback presence drift")
            if handler is not None:
                priority, = struct.unpack_from("<i", record, 0x50 + event * 16)
                if priority != handler["priority"]:
                    raise ValueError("copied raw callback priority drift")
        records.append((source_index, record))

    payload = serialize_records(records, bytes.fromhex(EXPECTED_DLL_SHA256))
    outfile.parent.mkdir(parents=True, exist_ok=True)
    with outfile.open("xb") as f:
        f.write(payload)
    return {
        "schema": 1,
        "kind": "private-linked-native-module-records",
        "source_count": SOURCE_COUNT,
        "active_record_count": RECORD_COUNT,
        "bytes_per_record": MODULE_SIZE,
        "file_bytes": len(payload),
        "fixture_sha256": hashlib.sha256(payload).hexdigest(),
        "dll_sha256": EXPECTED_DLL_SHA256,
        "original_native_addresses_present": True,
        "output_must_remain_private": True,
        "data_written_to_upstream_tree": False,
        "executable_code_executed": False,
        "all_171_registrations_verified_against_manifest": True,
    }


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--upstream", required=True)
    p.add_argument("--dll", required=True)
    p.add_argument("--map", required=True)
    p.add_argument("--private-out", required=True)
    args = p.parse_args(argv)
    result = export_records(args.upstream, args.dll, args.map, args.private_out)
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == "__main__":
    main()
