#!/usr/bin/env python3
"""Check static controller/test associations. Never claims behavior equivalence."""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LEDGER = "docs/R316_CONTROLLER_TEST_EVIDENCE.json"
HEADER_DIR = "reference_compat/include/slinkywinky"


def registered_test_sources(text, prefix=""):
    commands = set(re.findall(
        r"add_test\s*\(\s*NAME\s+\S+\s+COMMAND\s+([A-Za-z0-9_]+)(?=\s|\))",
        text))
    sources = set()
    for target, body in re.findall(
            r"add_executable\s*\(\s*([A-Za-z0-9_]+)\s+([^)]*)\)", text):
        if target not in commands:
            continue
        for path in re.findall(
                r"(?:^|\s)((?:reference_compat/)?tests/[A-Za-z0-9_]+\.cpp)(?=\s|$)",
                body):
            sources.add(prefix + path)
    return sources


def audit(root=ROOT):
    ledger = json.loads((root / LEDGER).read_text(encoding="utf-8"))
    if ledger["schema"] != 1:
        raise ValueError("unsupported evidence schema")
    associations = ledger["controllers"]
    headers = {p.name.removesuffix("_controller.hpp")
               for p in (root / HEADER_DIR).glob("*_controller.hpp")}
    if set(associations) != headers:
        raise ValueError("controller evidence roster mismatch")
    registered = registered_test_sources((root / "CMakeLists.txt").read_text(encoding="utf-8"))
    registered |= registered_test_sources(
        (root / "reference_compat/CMakeLists.txt").read_text(encoding="utf-8"),
        "reference_compat/")
    referenced = set()
    for name, paths in associations.items():
        if not isinstance(paths, list) or not paths or len(paths) != len(set(paths)):
            raise ValueError("empty or duplicate test association: " + name)
        for path in paths:
            if (not isinstance(path, str)
                    or not re.fullmatch(r"reference_compat/tests/[A-Za-z0-9_]+\.cpp", path)
                    or path not in registered or not (root / path).is_file()):
                raise ValueError("unregistered or absent test source: " + name + ": " + str(path))
            referenced.add(path)
    return {"controller_headers": len(headers),
            "associated_registered_test_sources": len(referenced),
            "runtime_parity_verified": False,
            "assertion_coverage_verified": False}


if __name__ == "__main__":
    print(json.dumps(audit(), sort_keys=True))
