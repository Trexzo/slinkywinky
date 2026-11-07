#!/usr/bin/env python3
"""Offline audit of the user's candidate recovered module registration table.

Usage:
    python tools/audit_recovered_registration.py \
        --archive "/path/to/decrypted files.zip"

No file extraction, network, code execution from the archive, or mutation.
The candidate blob is *not* authenticated native commercial Slinky source.
"""
import argparse
import collections
import json
import pathlib
import re
import sys
import zipfile

TABS = ("Combat", "Move", "Visual", "Block", "Utility")
CATALOG_RE = re.compile(
    r'\{"([^"]+)",\s*MenuTab::(Combat|Move|Visual|Block|Utility)\}'
)
EXPECTED_COUNTS = (17, 10, 15, 7, 18)


def audit(archive: pathlib.Path, catalog: pathlib.Path):
    with zipfile.ZipFile(archive) as zf:
        matching = [name for name in zf.namelist()
                    if name.endswith("/re/modules_real.json")]
        if len(matching) != 1:
            raise ValueError("one unambiguous re/modules_real.json required")
        info = zf.getinfo(matching[0])
        if info.file_size > 4_000_000:
            raise ValueError("registration JSON exceeds 4 MiB")
        source = json.loads(zf.read(info).decode("utf-8"))
    if not isinstance(source, list) or len(source) != 67:
        raise ValueError("expected 67 candidate registration descriptors")
    candidate = {}
    counts = collections.Counter()
    engine_only = []
    for record in source:
        if not isinstance(record, dict):
            raise ValueError("registration descriptor is not an object")
        name, cat, addr, callback = (
            record.get("name"), record.get("cat"),
            record.get("addr"), record.get("cb"))
        if (not isinstance(name, str) or not name or
            name in candidate or not isinstance(cat, int) or
            cat not in range(5) or not isinstance(addr, int) or
            addr <= 0 or not isinstance(callback, int) or callback < 0):
            raise ValueError("duplicate or malformed registration descriptor")
        candidate[name] = cat
        counts[cat] += 1
        if callback == 0:
            engine_only.append(name)
    parsed = CATALOG_RE.findall(catalog.read_text(encoding="utf-8"))
    if len(parsed) != 67:
        raise ValueError("expected exactly 67 C++ catalog descriptors")
    observed = {}
    for name, tab in parsed:
        if name in observed:
            raise ValueError("duplicate C++ catalog descriptor: " + name)
        observed[name] = TABS.index(tab)
    missing = sorted(name for name in candidate if name not in observed)
    extra = sorted(name for name in observed if name not in candidate)
    wrong_tab = sorted(
        [{"name": name, "candidate_tab": TABS[cat],
          "cpp_tab": TABS[observed[name]]}
         for name, cat in candidate.items()
         if name in observed and observed[name] != cat],
        key=lambda item: item["name"])
    expected = tuple(counts[i] for i in range(5))
    result = {
        "candidate_source": matching[0],
        "candidate_total": len(source),
        "catalog_total": len(observed),
        "category_counts": dict(zip(TABS, expected)),
        "zero_callback_modules": sorted(engine_only),
        "missing": missing,
        "extra": extra,
        "wrong_tab": wrong_tab,
        "pass": (not missing and not extra and not wrong_tab and
                 expected == EXPECTED_COUNTS and len(engine_only) == 6),
    }
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=pathlib.Path, required=True)
    parser.add_argument(
        "--catalog", type=pathlib.Path,
        default=pathlib.Path(__file__).resolve().parents[1] /
                "src/r115_recovered_catalog.inc")
    args = parser.parse_args()
    try:
        result = audit(args.archive, args.catalog)
    except (OSError, ValueError, zipfile.BadZipFile,
            UnicodeError, json.JSONDecodeError) as exc:
        print("AUDIT_ERROR:", exc, file=sys.stderr)
        return 2
    print(json.dumps(result, indent=2, ensure_ascii=False))
    return 0 if result["pass"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
