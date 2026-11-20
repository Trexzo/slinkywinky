#!/usr/bin/env python3
"""R242: independently check the shipped Clutch source table against
operator-supplied candidate decrypted Slinky .rdata (never execute binaries).

Usage:
  python tools/verify_r242_clutch_reference.py --archive '/path/to/decrypted files.zip'
  python tools/verify_r242_clutch_reference.py --self-test
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import re
import struct
import sys
import zipfile

RDATA_ENTRY = "decrypted files/slinky2.decrypted/rdata.bin"
RDATA_SHA256 = "61eaa6bad74fd5cf0d374198506ec4f87fa042b5cf673e37ae01e87eb336706a"
EXPECTED_COUNT = 167
EXPECTED_PREFIX = (1, 6, 19, 43, 79, 123, 163, 167)
EXPECTED_POSITION = 0x18280  # byte index in the extracted .rdata, not a PE RVA


def parse_header(text: str) -> bytes:
    def numbers(symbol: str) -> list[int]:
        match = re.search(r"\b" + re.escape(symbol) + r"\s*\{\{(.*?)\}\}", text, re.S)
        if match is None:
            raise ValueError(f"missing source table: {symbol}")
        return [int(value, 0) for value in
                re.findall(r"\b(?:0x[0-9a-fA-F]+|[0-9]+)(?=\s*[uU]?\b)",
                           match.group(1))]

    prefix = tuple(numbers("kClutchDistanceCounts"))
    offsets = numbers("kClutchSourceOffsets")
    if prefix != EXPECTED_PREFIX or len(offsets) != EXPECTED_COUNT:
        raise ValueError("source prefix or source entry count differs")
    if any(v < 0 or v > 0xFFF for v in offsets):
        raise ValueError("invalid packed source offset")
    return struct.pack("<8H", *prefix) + struct.pack("<167H", *offsets)


def verify_bytes(rdata: bytes, expected: bytes, enforce_pinned: bool = True) -> int:
    if enforce_pinned and hashlib.sha256(rdata).hexdigest() != RDATA_SHA256:
        raise ValueError("reference .rdata SHA-256 differs from pinned decrypted archive")
    location = rdata.find(expected)
    if location < 0:
        raise ValueError("source prefix/candidate table not found contiguously in reference")
    if rdata.find(expected, location + 1) >= 0:
        raise ValueError("ambiguous duplicate candidate table in reference")
    if enforce_pinned and location != EXPECTED_POSITION:
        raise ValueError("unexpected reference table offset")
    return location


def self_test(expected: bytes) -> None:
    base = b"before" + expected + b"after"
    if verify_bytes(base, expected, enforce_pinned=False) != len(b"before"):
        raise AssertionError("R242 fixture positive control failed")
    invalid = [
        b"unrelated bytes",
        b"before" + expected[:-1] + b"after",
        b"before" + bytes([expected[0] ^ 1]) + expected[1:] + b"after",
        b"before" + expected[:17] + bytes([expected[17] ^ 1]) + expected[18:] + b"after",
        base + expected,
    ]
    for case in invalid:
        try:
            verify_bytes(case, expected, enforce_pinned=False)
        except ValueError:
            continue
        raise AssertionError("R242 negative control unexpectedly admitted")
    print("R242_CLUTCH_DECRYPTED_REFERENCE_SELF_TEST_PASS negatives=5")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path,
                        help="operator-owned reference ZIP; never executed")
    parser.add_argument("--header", type=Path,
                        default=Path(__file__).resolve().parents[1] /
                        "reference_compat/include/slinkywinky/clutch_candidate_table.hpp")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if not args.self_test and args.archive is None:
        parser.error("supply --archive or --self-test")
    try:
        expected = parse_header(args.header.read_text(encoding="utf-8"))
        if len(expected) != 350:
            raise ValueError("unexpected source table byte size")
        if args.self_test:
            self_test(expected)
        if args.archive:
            with zipfile.ZipFile(args.archive) as archive:
                data = archive.read(RDATA_ENTRY)
            pos = verify_bytes(data, expected)
            table_sha = hashlib.sha256(expected[16:]).hexdigest()
            print(f"R242_CLUTCH_DECRYPTED_REFERENCE_EXACT_PASS "
                  f"distance_counts=8 candidates=167 byte_length={len(expected)} "
                  f"offset=0x{pos:x} candidate_sha256={table_sha}")
    except (OSError, zipfile.BadZipFile, KeyError, ValueError) as error:
        print(f"R242_CLUTCH_DECRYPTED_REFERENCE_REFUSED: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
