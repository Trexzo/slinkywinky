#!/usr/bin/env python3
"""R373: fail-closed native category/name guard coverage for offline binders.

Checks first-party code and metadata only, never loads private binaries.
The recovered Java class's category is not substituted for the native table.
"""
from pathlib import Path
import collections
import re

ROOT = Path(__file__).resolve().parents[1]
CATEGORIES = {"combat": 0, "move": 1, "visual": 2, "block": 3, "utility": 4}
EXPECTED_COUNTS = {"combat": 17, "move": 10, "visual": 15, "block": 7, "utility": 18}
DIRECT_BINDERS = 46

def require(condition, reason):
    if not condition:
        raise ValueError(reason)

def module_inventory(matrix_text):
    inventory = {}
    counts = collections.Counter()
    for line in matrix_text.splitlines():
        parts = [part.strip() for part in line.split("|")]
        if len(parts) < 6 or parts[1] not in CATEGORIES:
            continue
        name = parts[2].strip("`")
        require(bool(re.fullmatch(r"[a-z][a-z0-9_]*", name)) and name not in inventory,
                "invalid or duplicated native inventory entry")
        inventory[name] = CATEGORIES[parts[1]]
        counts[parts[1]] += 1
    require(len(inventory) == 67 and dict(counts) == EXPECTED_COUNTS,
            "native descriptor count/category drift")
    require(inventory.get("ghost_hand") == 3 and inventory.get("chest_stealer") == 4,
            "native Ghost Hand / descriptor-only category drift")
    return inventory

def functions(source):
    pattern = re.compile(r"\bbool\s+(bind_[a-z0-9_]+)\s*\(\s*std::size_t\s+id\s*\)\s*\{")
    found = {}
    for match in pattern.finditer(source):
        depth = 1
        pos = match.end()
        start = pos
        while depth and pos < len(source):
            if source[pos] == "{":
                depth += 1
            elif source[pos] == "}":
                depth -= 1
            pos += 1
        require(depth == 0, "unterminated binder " + match.group(1))
        require(match.group(1) not in found, "duplicate binder " + match.group(1))
        found[match.group(1)] = source[start:pos - 1]
    return found

def audit_sources(matrix_text, bridge_text):
    inventory = module_inventory(matrix_text)
    binder_map = functions(bridge_text)
    checked = {}
    suffixes = ("_source_tick", "_source_packet", "_source_frame",
                "_source_flush", "_source_render", "_source_attack",
                "_source_toggle")
    for binder, body in binder_map.items():
        if binder.endswith("_from_catalog"):
            continue  # wrapper delegates to a directly admitted binder
        module = binder[len("bind_"):]
        for suffix in suffixes:
            if module.endswith(suffix):
                module = module[:-len(suffix)]
                break
        require(module in inventory and module != "chest_stealer",
                "unrecognized native binder: " + binder)
        expected = inventory[module]
        guarded_categories = re.findall(
            r"definitions_\[id\]\.category\s*!=\s*(\d+)", body)
        source_rows = re.findall(
            r'matches_source_row\s*\(\s*id\s*,\s*"([^"]+)"\s*,\s*(\d+)', body)
        require(len(guarded_categories) + len(source_rows) == 1,
                "category admission missing or ambiguous: " + binder)
        actual = (int(guarded_categories[0]) if guarded_categories
                  else int(source_rows[0][1]))
        require(actual == expected,
                f"native category mismatch: {binder} expected={expected} actual={actual}")
        canonical = " ".join(word.capitalize() for word in module.split("_"))
        guarded_names = re.findall(
            r'definitions_\[id\]\.name\s*!=\s*"([^"]+)"', body)
        if source_rows:
            require(source_rows[0][0] == canonical,
                    "source descriptor name mismatch: " + binder)
        if guarded_names:
            require(len(guarded_names) == 1 and guarded_names[0] == canonical,
                    "direct descriptor name mismatch: " + binder)
        checked[binder] = module
    require(len(checked) == DIRECT_BINDERS,
            f"native binder coverage count drift: {len(checked)} not {DIRECT_BINDERS}")
    require("bind_ghost_hand" in checked and
            "bind_backtrack_source_tick" in checked and "bind_inv_walk_source_tick" in checked,
            "critical category binders absent")
    return {"descriptors": len(inventory),
            "registered": len(inventory) - 1,
            "native_direct_binders": len(checked),
            "module_names_with_binders": len(set(checked.values())),
            "ghost_hand_native_category": inventory["ghost_hand"],
            "chest_stealer_descriptor_only": True}

def main():
    report = audit_sources(
        (ROOT / "docs" / "MODULE_MATRIX.md").read_text(encoding="utf-8"),
        (ROOT / "reference_compat" / "include" / "slinkywinky" /
         "module_execution_bridge.hpp").read_text(encoding="utf-8"))
    print("R373_NATIVE_BINDER_CATEGORY_AUDIT_PASS", report)

if __name__ == "__main__":
    main()
