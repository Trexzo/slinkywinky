#!/usr/bin/env python3
"""Read-only comparison against the user's LOCAL candidate decrypted Slinky ZIP.

Never executes a DLL, injector or archive script; never writes binary content to
GitHub or the JSON report. Evidence is pinned to one candidate build, not an
authenticated official commercial Slinky release.
"""
import argparse
import hashlib
import json
import re
import struct
import sys
import zipfile
from pathlib import Path

TEXT_SHA256 = "35165183210c4f50cde535dc98b806b29ed7efd8fb3da3822f416915559302af"
RDATA_SHA256 = "61eaa6bad74fd5cf0d374198506ec4f87fa042b5cf673e37ae01e87eb336706a"
REACH_SHA256 = "851ce5208a7ae4a4a0b7c46b6e0487b9dff9821e005479cc7d375241fe9b2acb"
TEXT_RVA = 0x1000
REACH_RVA = 0x2FFB0
MANAGER_RVA = 0x28000
MAX_MEMBER_SIZE = 64 * 1024 * 1024


def sha(data):
    return hashlib.sha256(data).hexdigest()


def member(zf, suffix):
    names = [n for n in zf.namelist() if n.endswith(suffix)]
    if len(names) != 1:
        raise ValueError(f"Expected one ZIP member ending in {suffix!r}; found {len(names)}")
    info = zf.getinfo(names[0])
    if info.file_size > MAX_MEMBER_SIZE:
        raise ValueError(f"Oversized ZIP member: {suffix} ({info.file_size} bytes)")
    return zf.read(info)


def pe_sections(pe):
    if len(pe) < 0x40 or pe[:2] != b"MZ":
        raise ValueError("Embedded DLL is not a valid MZ image")
    nt_off = struct.unpack_from("<I", pe, 0x3C)[0]
    if nt_off + 24 > len(pe) or pe[nt_off:nt_off + 4] != b"PE\0\0":
        raise ValueError("Embedded DLL has no valid PE header")
    n = struct.unpack_from("<H", pe, nt_off + 6)[0]
    optional_size = struct.unpack_from("<H", pe, nt_off + 20)[0]
    if not 1 <= n <= 96:
        raise ValueError(f"Implausible PE section count: {n}")
    section_start = nt_off + 24 + optional_size
    if section_start + n * 40 > len(pe):
        raise ValueError("PE section table is out of bounds")
    result = {}
    for idx in range(n):
        off = section_start + idx * 40
        name = pe[off:off + 8].split(b"\0", 1)[0].decode("ascii", "replace")
        virtual_size, rva, raw_size, raw_offset = struct.unpack_from("<IIII", pe, off + 8)
        if raw_offset + raw_size > len(pe):
            raise ValueError(f"Section {name} exceeds the PE file")
        if name in result:
            raise ValueError(f"Duplicate PE section: {name}")
        result[name] = (virtual_size, rva, pe[raw_offset:raw_offset + raw_size])
    return result


def read_reference(archive):
    with zipfile.ZipFile(archive) as zf:
        text = member(zf, "/slinky2.decrypted/text.bin")
        rdata = member(zf, "/slinky2.decrypted/rdata.bin")
        dll = member(zf, "/slinky.dll")
    if sha(text) != TEXT_SHA256 or sha(rdata) != RDATA_SHA256:
        raise ValueError("Decrypted section SHA-256 differs from pinned candidate")
    sections = pe_sections(dll)
    for name, expected, rva in ((".text", text, TEXT_RVA), (".rdata", rdata, 0x13E000)):
        if name not in sections:
            raise ValueError(f"Missing {name} in embedded DLL")
        virtual_size, observed_rva, raw = sections[name]
        if virtual_size != len(expected) or observed_rva != rva or raw[:len(expected)] != expected:
            raise ValueError(f"Embedded DLL {name} differs from its extracted section")
    reach_off = REACH_RVA - TEXT_RVA
    if sha(text[reach_off:reach_off + 176]) != REACH_SHA256:
        raise ValueError("Reach instruction segment differs from pinned candidate")
    # Fixed addresses are interpreted only after verifying the entire .text hash.
    def imm32(rva, displacement=0):
        return struct.unpack_from("<I", text, rva - TEXT_RVA + displacement)[0]
    count = imm32(MANAGER_RVA + 0x2F, 4)
    calloc_count = imm32(MANAGER_RVA + 0x37, 1)
    copy_bytes = imm32(MANAGER_RVA + 0x5F, 2)
    loop_limit = text[MANAGER_RVA + 0x84 - TEXT_RVA + 3]
    if (count, calloc_count, copy_bytes, loop_limit) != (66, 66, 528, 66):
        raise ValueError("Candidate original manager instruction immediates are inconsistent")
    return {
        "binary_sha256": sha(dll),
        "text_sha256": sha(text),
        "rdata_sha256": sha(rdata),
        "reach_instructions_sha256": sha(text[reach_off:reach_off + 176]),
        "pe_sections_match_extracted_data": True,
        "candidate_manager_registered": count,
        "candidate_manager_allocation_entries": calloc_count,
        "candidate_manager_descriptor_copy_bytes": copy_bytes,
        "candidate_manager_loop_limit": loop_limit,
    }


def inspect_source(root):
    content = (root / "src/engine/jni/module_manager.cpp").read_text(encoding="utf-8")
    def match(regex, name):
        m = re.search(regex, content, re.MULTILINE)
        if not m:
            raise ValueError(f"Cannot locate {name} in C++ source")
        return int(m.group(1), 0)
    return {
        "source_manager_registered": match(r"manager->count\s*=\s*(0x[0-9a-fA-F]+|[0-9]+)", "manager count"),
        "source_manager_descriptor_copy_bytes": match(r"libc_memcpy_alt\(manager->mods,\s*ptr_to_aim_assist_module_descriptor,\s*(0x[0-9a-fA-F]+|[0-9]+)\)", "descriptor copy"),
        "source_manager_table_entries": match(r"ptr_to_aim_assist_module_descriptor\[(0x[0-9a-fA-F]+|[0-9]+)\]", "descriptor table"),
    }


def audit(archive, source):
    record = read_reference(archive)
    cpp = inspect_source(source)
    record.update(cpp)
    record["registration_delta"] = cpp["source_manager_registered"] - record["candidate_manager_registered"]
    record["descriptor_copy_delta_bytes"] = cpp["source_manager_descriptor_copy_bytes"] - record["candidate_manager_descriptor_copy_bytes"]
    record["source_descriptor_table_consistent"] = (
        cpp["source_manager_registered"] == cpp["source_manager_table_entries"]
        and cpp["source_manager_descriptor_copy_bytes"] == cpp["source_manager_registered"] * 8
    )
    if not record["source_descriptor_table_consistent"]:
        raise ValueError("C++ descriptor array/copy count is inconsistent")
    record["matches_candidate_manager"] = (
        record["registration_delta"] == 0 and record["descriptor_copy_delta_bytes"] == 0
    )
    record["executable_code_run"] = False
    record["official_provenance_authenticated"] = False
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate-zip", required=True, type=Path, help="Private local decrypted files.zip, never uploaded")
    parser.add_argument("--source", type=Path, default=Path("references/OpenSlinky"))
    parser.add_argument("--json-out", type=Path, help="Only hashes and counts, never binary bytes")
    parser.add_argument("--strict", action="store_true", help="Return 2 if C++ manager differs from candidate")
    args = parser.parse_args()
    try:
        result = audit(args.candidate_zip, args.source)
        if args.json_out:
            args.json_out.parent.mkdir(parents=True, exist_ok=True)
            args.json_out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(json.dumps(result, indent=2, sort_keys=True))
        if not result["matches_candidate_manager"]:
            print("KNOWN FIDELITY GAP: C++ module manager differs from candidate DLL", file=sys.stderr)
            return 2 if args.strict else 0
        return 0
    except (OSError, ValueError, zipfile.BadZipFile, struct.error) as exc:
        print(f"Candidate binary audit failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
