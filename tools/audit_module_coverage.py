#!/usr/bin/env python3
"""R258 independent source/module inventory consistency check; no game launch."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
matrix = ROOT / "docs" / "MODULE_MATRIX.md"
java_file = ROOT / "docs" / "RECOVERED_JAVA_MODULE_NAMES.txt"
headers_dir = ROOT / "reference_compat" / "include" / "slinkywinky"

def verify():
    rows = {}
    for line in matrix.read_text(encoding="utf-8").splitlines():
        columns = [s.strip() for s in line.split("|")]
        if len(columns) < 6 or columns[1] not in ("block", "combat", "move", "utility", "visual"):
            continue
        name = columns[2].strip(chr(96))
        if not re.fullmatch(r"[a-z][a-z0-9_]*", name) or name in rows:
            raise ValueError("invalid/duplicate module: " + repr(name))
        rows[name] = columns[3]
    java = [s for s in java_file.read_text(encoding="utf-8").splitlines()
            if s and not s.startswith("#")]
    if java != sorted(set(java)) or len(java) != 68:
        raise ValueError("Java recovery snapshot changed")
    java_only = set(java) - set(rows)
    if len(rows) != 67 or set(rows) - set(java) or java_only != {"speed"}:
        raise ValueError("Native/Java source-name mismatch: " + repr(sorted(java_only)))
    headers = {p.name[:-len("_controller.hpp")] for p in
               headers_dir.glob("*_controller.hpp")}
    if headers - set(rows):
        raise ValueError("Controller missing native inventory row")
    for name, state in rows.items():
        if (state.startswith("Offline C++")) != (name in headers):
            raise ValueError("Controller status drift: " + name + ": " + state)
    experimental = [name for name, state in rows.items()
                    if state == "Experimental decision model (offline)"]
    missing = [name for name, state in rows.items()
               if state == "Not implemented"]
    if len(experimental) != 6 or len(missing) != 36 or (
        len(headers) + len(experimental) + len(missing) != len(rows)
    ):
        raise ValueError("Coverage accounting or unknown status")
    print("R258_RECONSTRUCTION_COVERAGE_PASS "
          "native=67 java=68 controllers=%d experimental=6 missing=36 "
          "java_only=speed" % len(headers))
    print("R258_MISSING_MODULES " + ",".join(sorted(missing)))

if __name__ == "__main__":
    verify()
