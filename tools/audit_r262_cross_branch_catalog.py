#!/usr/bin/env python3
"""R262: compare PR1 native GUI catalog to PR2's current module inventory.

This compares source metadata only; no binary reference or Minecraft launch.
"""
import argparse
import collections
from pathlib import Path
import re

TABS = {"Combat": "combat", "Move": "move", "Visual": "visual",
        "Block": "block", "Utility": "utility"}
CATEGORY_COUNTS = {"combat": 17, "move": 10, "visual": 15,
                   "block": 7, "utility": 18}
EXPERIMENTAL = {"aim_assist", "auto_block", "auto_clicker",
                "hit_select", "reach", "velocity"}

def require(ok, message):
    if not ok:
        raise ValueError(message)

def normalized(display):
    return re.sub(r"[^a-z0-9]+", "_", display.lower()).strip("_")

def parse_gui_catalog(text):
    # Source catalog is a literal C++ array. Require complete, bounded entries.
    begin = text.find("constexpr std::array<MenuModule,67>")
    require(begin >= 0, "PR1 67-descriptor GUI array missing")
    source = text[begin:]
    entries = re.findall(r'\{\s*"([^"]+)"\s*,\s*MenuTab::(Combat|Move|Visual|Block|Utility)\s*\}', source)
    require(len(entries) == 67, f"GUI catalog entry count drift: {len(entries)}")
    rows = {}
    counts = collections.Counter()
    for title, tab in entries:
        name = normalized(title)
        require(name and name not in rows, f"duplicate/invalid GUI module {name!r}")
        rows[name] = TABS[tab]
        counts[TABS[tab]] += 1
    require(dict(counts) == CATEGORY_COUNTS, "GUI source category count drift")
    return rows

def parse_pr2_matrix(text):
    rows = {}
    counts = collections.Counter()
    for line in text.splitlines():
        p = [part.strip() for part in line.split("|")]
        if len(p) < 6 or p[1] not in TABS.values():
            continue
        name = p[2].strip("`")
        require(bool(re.fullmatch(r"[a-z][a-z0-9_]*", name)) and name not in rows,
                "duplicate/invalid PR2 native row")
        rows[name] = (p[1], p[3])
        counts[p[1]] += 1
    require(len(rows) == 67 and dict(counts) == CATEGORY_COUNTS,
            "PR2 native inventory count/category drift")
    return rows

def audit(gui_text, matrix_text, headers):
    gui = parse_gui_catalog(gui_text)
    native = parse_pr2_matrix(matrix_text)
    require(set(gui) == set(native),
            f"cross-branch descriptor names differ: PR1_only={sorted(set(gui)-set(native))} "
            f"PR2_only={sorted(set(native)-set(gui))}")
    mismatches = [n for n in gui if gui[n] != native[n][0]]
    require(not mismatches, "GUI-to-native category disagreement: " + ",".join(mismatches))
    require(native["ghost_hand"][0] == "block",
            "Ghost Hand native Block category not preserved")
    require(native["chest_stealer"][1] == "Not implemented",
            "descriptor-only Chest Stealer improperly promoted")
    registered = set(gui) - {"chest_stealer"}
    header_set = set(headers)
    require(len(header_set) == 60 and header_set <= registered,
            "PR2 offline-controller header inventory drift")
    experimental = {name for name, (_, state) in native.items()
                    if state == "Experimental decision model (offline)"}
    missing = {name for name, (_, state) in native.items()
               if state == "Not implemented"}
    require(experimental == EXPERIMENTAL and missing == {"chest_stealer"},
            "PR2 experimental/descriptor-only classification drift")
    require(registered == header_set | experimental and
            not (header_set & experimental), "PR2 registered-66 source coverage drift")
    return {"gui_descriptors": len(gui), "native_registered": len(registered),
            "partial_controller_headers": len(header_set),
            "experimental_models": len(experimental),
            "descriptor_only": "chest_stealer",
            "category_mismatches": 0}

def read_headers(pr2_root):
    root = Path(pr2_root) / "reference_compat" / "include" / "slinkywinky"
    require(root.is_dir(), "PR2 controller header directory missing")
    return {p.name[:-len("_controller.hpp")]
            for p in root.glob("*_controller.hpp")}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pr1", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--pr2", type=Path, required=True)
    args = parser.parse_args()
    result = audit(
        (args.pr1 / "src" / "r115_recovered_catalog.inc").read_text(encoding="utf-8"),
        (args.pr2 / "docs" / "MODULE_MATRIX.md").read_text(encoding="utf-8"),
        read_headers(args.pr2))
    print("R262_CROSS_BRANCH_NATIVE_CATALOG_PASS", result)

if __name__ == "__main__":
    main()
