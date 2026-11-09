#!/usr/bin/env python3
"""Export an offline 66-module event fixture from a PRIVATE candidate Slinky ZIP.

Writes only module names, categories, event-slot numbers and numeric priority
metadata; NEVER writes original native instructions, table pointer values,
embedded binaries or extracted code. The original ZIP stays local.
"""
import argparse
import collections
import csv
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path

BASE = 0x180000000
RDATA_RVA = 0x13E000
DATA_RVA = 0x2E5000
TEXT_RVA = 0x1000
TEXT_SIZE = 1295174
DESCRIPTOR_TABLE_RVA = 0x14F810
N_MODULES = 66
EVENT_SLOTS = 27
DATA_SHA256 = "8aee98d473f5092686ea3ed080828bbb02500e4bbd8d89130a3ce7751176dea5"
RDATA_SHA256 = "61eaa6bad74fd5cf0d374198506ec4f87fa042b5cf673e37ae01e87eb336706a"
CATEGORY_COUNTS = {0: 17, 1: 10, 2: 15, 3: 7, 4: 17}
TOTAL_CALLBACKS = 171


def get_member(zf, suffix, max_bytes):
    matching = [i for i in zf.infolist() if i.filename.endswith(suffix)]
    if len(matching) != 1 or matching[0].file_size > max_bytes:
        raise ValueError(f"Expected exactly one bounded ZIP member {suffix}")
    return zf.read(matching[0])


def image_offset(pointer, rva, size, required=1):
    off = pointer - BASE - rva
    if off < 0 or off + required > size:
        raise ValueError(f"Pointer outside candidate image region 0x{rva:x}")
    return off


def read_catalog(data, rdata):
    table_off = DESCRIPTOR_TABLE_RVA - RDATA_RVA
    if table_off < 0 or table_off + N_MODULES * 8 > len(rdata):
        raise ValueError("Candidate module descriptor table outside .rdata")
    pointers = struct.unpack_from(f"<{N_MODULES}Q", rdata, table_off)
    if len(set(pointers)) != N_MODULES:
        raise ValueError("Repeated native descriptor pointers")
    rows = []
    for index, ptr in enumerate(pointers):
        off = image_offset(ptr, DATA_RVA, len(data), 0x48 + EVENT_SLOTS * 16)
        nameptr = struct.unpack_from("<Q", data, off)[0]
        nameoff = image_offset(nameptr, RDATA_RVA, len(rdata))
        end = rdata.find(b"\0", nameoff, min(len(rdata), nameoff + 128))
        if end == -1:
            raise ValueError(f"Unterminated module name, index {index}")
        name = rdata[nameoff:end].decode("utf-8")
        if not name or any(c in name for c in "\t\r\n"):
            raise ValueError("Malformed module name")
        category = struct.unpack_from("<I", data, off + 0x10)[0]
        enabled = data[off + 0x14]
        if category not in CATEGORY_COUNTS or enabled not in (0, 1):
            raise ValueError(f"Invalid category or initial enabled state for {name}")
        subscriptions = []
        for event in range(EVENT_SLOTS):
            callback = struct.unpack_from("<Q", data, off + 0x48 + event * 16)[0]
            priority = struct.unpack_from("<i", data, off + 0x50 + event * 16)[0]
            if callback:
                image_offset(callback, TEXT_RVA, TEXT_SIZE)
                subscriptions.append((event, priority))
        rows.append({"index": index, "name": name, "category": category,
                     "initially_enabled": enabled, "subscriptions": subscriptions})
    if len({entry["name"] for entry in rows}) != N_MODULES:
        raise ValueError("Duplicate candidate module names")
    counts = collections.Counter(entry["category"] for entry in rows)
    total = sum(len(entry["subscriptions"]) for entry in rows)
    if dict(counts) != CATEGORY_COUNTS or total != TOTAL_CALLBACKS:
        raise ValueError(f"Candidate callback/catalog drift: {dict(counts)}; {total}")
    no_handler = {r["name"] for r in rows if not r["subscriptions"]}
    if no_handler != {"Chams", "No Hurt Cam"}:
        raise ValueError(f"Unexpected modules without native callbacks: {sorted(no_handler)}")
    return rows


def verify_known_original_bindings(rows):
    """Pinned private-candidate descriptor contracts; no original code copied."""
    expected = (
        (31, "Instant Stop", 1, [(16, 200)]),
        (35, "Item Use Fix", 4, [(14, 0), (15, 0)]),
        (42, "No Hit Delay", 4, [(14, 0)]),
        (45, "No Jump Delay", 1, [(17, 100)]),
        (46, "No Slow", 1, [(21, 200)]),
        (48, "No Use Delay", 4, [(18, 0)]),
        (49, "Null Move", 1, [(16, 100)]),
        (51, "Ping Fix", 4, [(2, -1000), (3, -1000)]),
        (54, "Reach", 0, [(8, 0)]),
    )
    for index, name, category, subscriptions in expected:
        if (len(rows) <= index or rows[index]["index"] != index or
                rows[index]["name"] != name or
                rows[index]["category"] != category or
                rows[index]["subscriptions"] != subscriptions):
            raise ValueError(f"Original candidate binding drift at module index {index}: {name}")


def export(archive, tsv_path, report_path=None):
    with zipfile.ZipFile(archive) as zf:
        data = get_member(zf, "/slinky2.decrypted/data.bin", 512 * 1024)
        rdata = get_member(zf, "/slinky2.decrypted/rdata.bin", 4 * 1024 * 1024)
    if hashlib.sha256(data).hexdigest() != DATA_SHA256:
        raise ValueError("Decrypted .data differs from pinned candidate")
    if hashlib.sha256(rdata).hexdigest() != RDATA_SHA256:
        raise ValueError("Decrypted .rdata differs from pinned candidate")
    rows = read_catalog(data, rdata)
    verify_known_original_bindings(rows)
    histogram = collections.Counter(event for row in rows for event, _ in row["subscriptions"])
    tsv_path.parent.mkdir(parents=True, exist_ok=True)
    with tsv_path.open("w", encoding="utf-8", newline="") as out:
        writer = csv.writer(out, dialect="excel-tab")
        writer.writerow(("id", "name", "category", "initially_enabled", "event_priorities"))
        for item in rows:
            pairs = ",".join(f"{event}:{priority}" for event, priority in item["subscriptions"])
            writer.writerow((item["index"], item["name"], item["category"], item["initially_enabled"], pairs))
    report = {"candidate_authenticated_as_official_release": False,
              "candidate_data_sha256": DATA_SHA256, "candidate_rdata_sha256": RDATA_SHA256,
              "registered_modules": len(rows), "callback_subscriptions": TOTAL_CALLBACKS,
              "module_category_counts": {str(k): v for k, v in sorted(CATEGORY_COUNTS.items())},
              "event_handler_counts": [histogram[e] for e in range(EVENT_SLOTS)],
              "no_native_handler_modules": ["Chams", "No Hurt Cam"],
              "original_executable_run": False, "fixture_intended_for_offline_testing_only": True}
    if report_path:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--candidate-zip", type=Path, required=True)
    p.add_argument("--tsv-out", type=Path, required=True)
    p.add_argument("--json-out", type=Path)
    args = p.parse_args()
    try:
        print(json.dumps(export(args.candidate_zip, args.tsv_out, args.json_out), indent=2))
        return 0
    except (OSError, ValueError, UnicodeError, struct.error, zipfile.BadZipFile) as exc:
        print(f"Candidate fixture export failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
