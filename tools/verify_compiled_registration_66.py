#!/usr/bin/env python3
"""R194: independent, static audit of compiled OpenSlinky registration fidelity.

Never loads or injects either DLL. Compares complete linked descriptor-pointer
tables to linker maps, and checks constructor/queue machine instructions in the
compiled COFF objects. The upstream source and built DLLs remain local.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

from prepare_openslinky_registration_66 import (
    RDATA_SHA256, RDATA_REL, UPSTREAM_SHA, descriptor_entries,
)

TABLE_SYMBOL = "ptr_to_aim_assist_module_descriptor"
EXTRA_INDEX = 17
SOURCE_COUNT = 67
EXPECTED_COUNT = 66
MAP_SYMBOL = re.compile(
    r"^\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(\S+)\s+([0-9A-Fa-f]{16})\s+\S+",
    re.MULTILINE,
)
FUNC_HEADING = re.compile(r"^[0-9A-Fa-f]{8,16} <([A-Za-z_]\w*)>:$", re.MULTILINE)


def sha256(payload):
    return hashlib.sha256(payload).hexdigest()


def parse_map_symbols(source, required_names):
    symbols = {}
    required = set(required_names) | {TABLE_SYMBOL}
    for match in MAP_SYMBOL.finditer(source):
        name, address = match.groups()
        value = int(address, 16)
        # Link maps can legitimately list identically named non-descriptor
        # helper symbols from different COMDATs. Descriptor/table ambiguity
        # is never tolerated.
        if name in symbols and symbols[name] != value:
            if name in required:
                raise ValueError("linker map has conflicting values for " + name)
            continue
        symbols[name] = value
    if TABLE_SYMBOL not in symbols:
        raise ValueError("linker map lacks static candidate table")
    return symbols


def map_rva_to_offset(blob, rva, length):
    if len(blob) < 0x100 or blob[:2] != b"MZ":
        raise ValueError("not a DOS/PE file")
    nt_offset, = struct.unpack_from("<I", blob, 0x3C)
    if nt_offset + 24 > len(blob) or blob[nt_offset:nt_offset + 4] != b"PE\x00\x00":
        raise ValueError("invalid PE signature")
    machine, number_sections = struct.unpack_from("<HH", blob, nt_offset + 4)
    size_optional, = struct.unpack_from("<H", blob, nt_offset + 20)
    opt = nt_offset + 24
    if machine != 0x8664 or size_optional < 0x70 or opt + size_optional > len(blob):
        raise ValueError("not a valid PE32+ AMD64 DLL")
    magic, = struct.unpack_from("<H", blob, opt)
    if magic != 0x20B:
        raise ValueError("not a PE32+ image")
    image_base, = struct.unpack_from("<Q", blob, opt + 24)
    if image_base == 0:
        raise ValueError("image base absent")
    start = opt + size_optional
    if not 1 <= number_sections <= 96 or start + number_sections * 40 > len(blob):
        raise ValueError("section headers truncated")
    for i in range(number_sections):
        off = start + 40 * i
        name = blob[off:off + 8].rstrip(b"\x00")
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<IIII", blob, off + 8)
        if virtual_address <= rva and rva + length <= virtual_address + min(virtual_size, raw_size):
            absolute = raw_offset + rva - virtual_address
            if absolute + length > len(blob):
                raise ValueError("PE section data truncated")
            return image_base, absolute, name.decode("ascii")
    raise ValueError("requested descriptor array is not entirely within raw PE sections")


def audit_linked_pointer_table(dll_path, map_path, source_entries):
    dll = Path(dll_path).read_bytes()
    symbols = parse_map_symbols(Path(map_path).read_text(encoding="utf-8"), source_entries)
    if len(source_entries) != SOURCE_COUNT:
        raise ValueError("not a 67-entry pinned source table")
    table_address = symbols[TABLE_SYMBOL]
    # Read PE header before mapping the requested image-relative table offset.
    nt = struct.unpack_from("<I", dll, 0x3C)[0] if len(dll) >= 0x40 else 0
    if nt + 24 + 32 > len(dll) or dll[nt:nt + 4] != b"PE\x00\x00":
        raise ValueError("invalid candidate DLL")
    imagebase = struct.unpack_from("<Q", dll, nt + 24 + 24)[0]
    if table_address < imagebase:
        raise ValueError("linker map address falls below PE image base")
    actual_base, raw_offset, section = map_rva_to_offset(
        dll, table_address - imagebase, SOURCE_COUNT * 8,
    )
    if actual_base != imagebase or section != ".rdata":
        raise ValueError("descriptor array is not in the expected PE .rdata")
    linked = struct.unpack_from("<" + "Q" * SOURCE_COUNT, dll, raw_offset)
    expected = []
    for name in source_entries:
        if name not in symbols:
            raise ValueError("linked module descriptor symbol missing: " + name)
        expected.append(symbols[name])
    for index, (actual, wanted) in enumerate(zip(linked, expected)):
        if actual != wanted:
            raise ValueError("linked descriptor pointer mismatch at index " + str(index))
    if source_entries[EXTRA_INDEX] != "chest_stealer_module_descriptor":
        raise ValueError("Chest Stealer original discrepancy shifted")
    return {
        "dll_sha256": sha256(dll),
        "size_bytes": len(dll),
        "image_base": hex(imagebase),
        "linked_static_entries": len(linked),
        "table_rva": hex(table_address - imagebase),
        "table_raw_offset": raw_offset,
        "table_bytes_verified": SOURCE_COUNT * 8,
        "all_linked_descriptor_addresses_match_map": True,
        "chest_stealer_static_index": EXTRA_INDEX,
        "model_compacted_registration_count": len(linked[:EXTRA_INDEX] + linked[EXTRA_INDEX+1:]),
    }


def disassembled_functions(output):
    markers = list(FUNC_HEADING.finditer(output))
    functions = {}
    for i, match in enumerate(markers):
        end = markers[i + 1].start() if i + 1 < len(markers) else len(output)
        if match.group(1) in functions:
            raise ValueError("ambiguous disassembly symbol " + match.group(1))
        functions[match.group(1)] = output[match.end():end]
    return functions


def require_instruction(block, expression, label):
    if not re.search(expression, block):
        raise ValueError("COFF instructions disagree with expected contract: " + label)


def audit_manager_instructions(disassembly, corrected):
    fs = disassembled_functions(disassembly)
    if "create_module_manager" not in fs or "queue_module_toggle" not in fs:
        raise ValueError("manager or toggle-queue COFF symbol missing")
    manager = fs["create_module_manager"]
    queue = fs["queue_module_toggle"]
    count = 0x42 if corrected else 0x43
    # This is tied to LLVM clang-cl 23.1.5's actual x64 lowering, not merely
    # a source-level integer grep; other compilers should fail closed.
    require_instruction(manager, rf"movq\s+\$0x{count:x},\s*0x8\(%rdi\)", "manager stored count")
    require_instruction(manager, rf"movl\s+\$0x{count:x},\s*%ecx", "manager calloc count")
    if corrected:
        require_instruction(manager, r"movl\s+\$0x88,\s*%r8d", "first 17 pointer bytes")
        require_instruction(manager, r"movl\s+\$0x188,\s*%r8d", "remaining 49 pointer bytes")
        require_instruction(manager, r"leaq\s+0x90\(%rip\),\s*%rdx", "skip index 17 in source pointer table")
        require_instruction(manager, r"cmpq\s+\$0x42,\s*%rcx", "loop bound 66")
        if re.search(r"movl\s+\$0x218,\s*%r8d", manager):
            raise ValueError("old 67-entry contiguous copy still present")
        # Optimization compares against 65 for >=66 on unsigned queue count.
        require_instruction(queue, r"cmpq\s+\$0x41,\s*%r8", "queue saturation at 66")
        if len(re.findall(r"cmpq\s+\$0x41,\s*%r8", queue)) < 2:
            raise ValueError("both enable and disable queue bounds must be 66")
        return {"stored_module_count": 66, "allocation_count": 66,
                "pointer_copy_lengths": [136, 392],
                "source_skip_bytes": 144, "queue_limit": 66}
    require_instruction(manager, r"movl\s+\$0x218,\s*%r8d", "original contiguous 536-byte copy")
    require_instruction(queue, r"cmpq\s+\$0x42,\s*%r8", "original queue saturation at 67")
    return {"stored_module_count": 67, "allocation_count": 67,
            "pointer_copy_lengths": [536], "source_skip_bytes": 0,
            "queue_limit": 67}


def objdump_disassembly(executable, object_file):
    proc = subprocess.run(
        [str(executable), "--disassemble", str(object_file)],
        capture_output=True, text=True, check=False, timeout=30,
    )
    if proc.returncode != 0:
        raise ValueError("llvm-objdump refused COFF input: " + proc.stderr[:240])
    return proc.stdout


def audit(upstream, original_build, corrected_build, objdump):
    upstream = Path(upstream)
    source_bytes = (upstream / RDATA_REL).read_bytes()
    if sha256(source_bytes) != RDATA_SHA256:
        raise ValueError("pinned source rdata fingerprint differs")
    table = descriptor_entries(source_bytes.decode("utf-8"))
    if len(table) != SOURCE_COUNT:
        raise ValueError("unexpected upstream pointer count")
    outputs = {}
    for name, directory in [("original_67", original_build), ("corrected_66", corrected_build)]:
        directory = Path(directory)
        dll = directory / "slinky.dll"
        linkmap = directory / "slinky.map"
        obj = directory / "CMakeFiles/slinky.dir/src/engine/jni/module_manager.cpp.obj"
        if not all(p.is_file() for p in (dll, linkmap, obj)):
            raise ValueError("missing native build artifact for " + name)
        image = audit_linked_pointer_table(dll, linkmap, table)
        instructions = audit_manager_instructions(
            objdump_disassembly(objdump, obj), name == "corrected_66",
        )
        outputs[name] = {"linked_image": image, "compiled_manager": instructions}
    # The static 67-entry source table persists in both, but only the patched
    # manager compacts its private runtime registration list to 66.
    return {"schema": 1, "audit": "R194_static_compiled_registration",
            "upstream_commit": UPSTREAM_SHA,
            "source_rdata_sha256": RDATA_SHA256,
            "extra_registration_index": EXTRA_INDEX,
            "candidate_original_registration_count": EXPECTED_COUNT,
            "results": outputs, "dll_executed": False,
            "native_manager_runtime_verified": False,
            "original_commercial_parity_verified": False}


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--upstream", required=True)
    p.add_argument("--original-build", required=True)
    p.add_argument("--corrected-build", required=True)
    p.add_argument("--llvm-objdump", default="llvm-objdump")
    p.add_argument("--json-out")
    args = p.parse_args(argv)
    result = audit(args.upstream, args.original_build, args.corrected_build, args.llvm_objdump)
    payload = json.dumps(result, sort_keys=True, indent=2) + "\n"
    if args.json_out:
        target = Path(args.json_out)
        if target.exists():
            raise FileExistsError("refusing to replace an existing evidence file")
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(payload, encoding="utf-8")
    print(payload, end="")


if __name__ == "__main__":
    main()
