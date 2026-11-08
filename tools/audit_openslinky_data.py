#!/usr/bin/env python3
"""R173: read-only COFF relocation / decrypted .data parity audit.

Input COFF data object is assembled but never linked or executed. Optional
private candidate ZIP stays local; output contains only metadata and hashes.
"""
import argparse
import collections
import hashlib
import json
import re
import struct
import sys
import zipfile
from pathlib import Path

PIN = "fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4"
ORIGINAL_DATA_SHA256 = "8aee98d473f5092686ea3ed080828bbb02500e4bbd8d89130a3ce7751176dea5"
IMAGE_BASE = 0x180000000
DATA_RVA = 0x2E5000
DATA_LEN = 0x21608
EXPECTED_SOURCE_LEN = 0x21688
EXPECTED_TOTAL_RELOCS = 2442
EXPECTED_ORIGINAL_RELOCS = 2438
EXPECTED_APPENDED_RELOCS = 4
RELOC_ABS64 = 1


def sha256(data):
    return hashlib.sha256(data).hexdigest()


class COFF:
    def __init__(self, binary):
        self.raw = binary
        if len(binary) < 20:
            raise ValueError("COFF object truncated")
        machine, nsections, _, self.symbols_offset, self.nsymbols, optsize, _ = struct.unpack_from('<HHIIIHH', binary)
        if machine != 0x8664 or not (1 <= nsections <= 64) or optsize:
            raise ValueError("Not an expected Windows x64 COFF object")
        self.sections = {}
        if len(binary) < 20 + nsections * 40:
            raise ValueError("COFF section table truncated")
        for i in range(nsections):
            off = 20 + i * 40
            name = binary[off:off + 8].rstrip(b'\0').decode('ascii', errors='strict')
            size, raw_off, reloc_off = struct.unpack_from('<III', binary, off + 16)
            nreloc = struct.unpack_from('<H', binary, off + 32)[0]
            if raw_off + size > len(binary) or reloc_off + nreloc * 10 > len(binary):
                raise ValueError(f"COFF section {name} exceeds file bounds")
            if name in self.sections:
                raise ValueError(f"Duplicate COFF section {name}")
            relocs = [struct.unpack_from('<IIH', binary, reloc_off + j * 10) for j in range(nreloc)]
            self.sections[name] = {"index": i + 1, "bytes": binary[raw_off:raw_off + size], "relocs": relocs}
        str_start = self.symbols_offset + self.nsymbols * 18
        if str_start + 4 > len(binary):
            raise ValueError("COFF symbol/string table truncated")
        str_size = struct.unpack_from('<I', binary, str_start)[0]
        if str_size < 4 or str_start + str_size > len(binary):
            raise ValueError("Invalid COFF string table")
        self.strings = binary[str_start:str_start + str_size]

    def symbol(self, index):
        if index >= self.nsymbols:
            raise ValueError(f"Out-of-range COFF symbol index {index}")
        raw = self.raw[self.symbols_offset + index * 18:self.symbols_offset + index * 18 + 18]
        if raw[:4] == b'\x00' * 4:
            name_off = struct.unpack_from('<I', raw, 4)[0]
            if name_off < 4 or name_off >= len(self.strings):
                raise ValueError("Invalid COFF long-name offset")
            end = self.strings.find(b'\x00', name_off)
            if end == -1:
                raise ValueError("Unterminated COFF symbol name")
            name = self.strings[name_off:end].decode('utf-8')
        else:
            name = raw[:8].rstrip(b'\x00').decode('utf-8')
        value, section_num = struct.unpack_from('<Ih', raw, 8)
        return name, value, section_num


def address_map(source_root):
    mapping = collections.defaultdict(set)
    for filename in ("docs/global-symbol-map.json", "docs/symbol-map.json"):
        aliases = json.loads((source_root / filename).read_text(encoding="utf-8"))
        for original, alias in aliases.items():
            match = re.fullmatch(r'(?:data|rdata|fn|logic|cb|reg|aux)_([a-fA-F0-9]+)', original)
            if match:
                mapping[alias].add(int(match.group(1), 16))
    return mapping


def original_candidate_data(archive):
    with zipfile.ZipFile(archive) as z:
        paths = [name for name in z.namelist() if name.endswith('/slinky2.decrypted/data.bin')]
        if len(paths) != 1:
            raise ValueError(f"Expected one decrypted .data member, found {len(paths)}")
        info = z.getinfo(paths[0])
        if info.file_size > 512 * 1024:
            raise ValueError("Unexpected private .data size")
        data = z.read(info)
    if len(data) != DATA_LEN or sha256(data) != ORIGINAL_DATA_SHA256:
        raise ValueError("Private candidate decrypted .data differs from pinned build")
    return data


def audit(source_root, objfile, original_zip=None):
    obj = COFF(objfile.read_bytes())
    if '.data' not in obj.sections or obj.sections.get('.text', {}).get('bytes'):
        raise ValueError("Object must contain data without executable text bytes")
    sec = obj.sections['.data']
    compiled = sec['bytes']
    if len(compiled) != EXPECTED_SOURCE_LEN:
        raise ValueError(f"Unexpected .data section length: {len(compiled)}")
    reloc = sec['relocs']
    if len(reloc) != EXPECTED_TOTAL_RELOCS:
        raise ValueError(f"Unexpected absolute pointer relocation count: {len(reloc)}")
    before = [(off, index) for off, index, typ in reloc if off < DATA_LEN]
    after = [(off, index) for off, index, typ in reloc if off >= DATA_LEN]
    if len(before) != EXPECTED_ORIGINAL_RELOCS or len(after) != EXPECTED_APPENDED_RELOCS:
        raise ValueError("Original/appended pointer relocation boundary changed")
    if any(typ != RELOC_ABS64 or off + 8 > len(compiled) or off % 8 != 0
           for off, _, typ in reloc):
        raise ValueError("Unexpected non-absolute, misaligned or out-of-range relocation")
    if len({off for off, _, _ in reloc}) != len(reloc):
        raise ValueError("Duplicate relocation offsets")
    result = {
        "upstream_commit": PIN,
        "compiled_data_bytes": len(compiled),
        "original_candidate_data_bytes_expected": DATA_LEN,
        "appended_source_data_bytes": len(compiled) - DATA_LEN,
        "total_absolute_pointer_relocations": len(reloc),
        "pointer_relocations_in_candidate_range": len(before),
        "pointer_relocations_in_appended_range": len(after),
        "candidate_original_authenticated": False,
        "native_executable_run": False,
        "private_candidate_tested": original_zip is not None,
        "non_pointer_byte_mismatches": None,
        "original_pointer_targets_verified": None,
    }
    if original_zip is None:
        return result
    original = original_candidate_data(original_zip)
    norm_original = bytearray(original)
    norm_source = bytearray(compiled[:DATA_LEN])
    aliases = address_map(source_root)
    mismatched_ptr = []
    for offset, index in before:
        name, value, section_num = obj.symbol(index)
        targets = set(aliases.get(name, ()))
        if section_num == sec['index']:
            targets.add(DATA_RVA + value)
        addend = struct.unpack_from('<q', compiled, offset)[0]
        actual = struct.unpack_from('<Q', original, offset)[0]
        if not targets or actual not in {IMAGE_BASE + target + addend for target in targets}:
            mismatched_ptr.append((offset, name))
        norm_original[offset:offset + 8] = b'\0' * 8
        norm_source[offset:offset + 8] = b'\0' * 8
    diff = sum(a != b for a, b in zip(norm_original, norm_source))
    result['non_pointer_byte_mismatches'] = diff
    result['original_pointer_targets_verified'] = len(before) - len(mismatched_ptr)
    result['private_candidate_data_sha256'] = ORIGINAL_DATA_SHA256
    result['original_pointer_target_mismatches'] = len(mismatched_ptr)
    result['original_data_parity_after_relocations'] = diff == 0 and not mismatched_ptr
    if diff or mismatched_ptr:
        raise ValueError(f"Private .data parity failed: {diff} non-pointer bytes, {len(mismatched_ptr)} pointer targets")
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, default=Path('references/OpenSlinky'))
    p.add_argument('--object', type=Path, required=True,
                   help='Assembled data.s COFF object, not a DLL or executable')
    p.add_argument('--candidate-zip', type=Path, help='Optional PRIVATE decrypted ZIP; never upload')
    p.add_argument('--json-out', type=Path)
    args = p.parse_args()
    try:
        result = audit(args.source, args.object, args.candidate_zip)
        if args.json_out:
            args.json_out.parent.mkdir(parents=True, exist_ok=True)
            args.json_out.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n', encoding='utf-8')
        print(json.dumps(result, sort_keys=True, indent=2))
        return 0
    except (OSError, ValueError, struct.error, zipfile.BadZipFile) as exc:
        print(f'R173 data parity audit failed: {exc}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
