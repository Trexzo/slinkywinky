#!/usr/bin/env python3
"""Static two-archive reconciliation; reads only, never loads native code."""
import argparse
import collections
import hashlib
import json
import re
import struct
import zipfile
from pathlib import Path

PINS = ("326efa346905370ed1815aba2247d35666008346dd6858262c38070d0f42aea4",
        "c06b4c6033e2d19de86bad8f19127b0a3a68cfc496130233ebc9ff29f4540915")
CATEGORIES = ("combat", "move", "visual", "block", "utility")
HISTOGRAM = (24,5,6,10,3,25,7,4,4,3,3,2,1,5,10,6,3,9,20,1,1,3,2,1,1,5,7)
BASE = 0x180000000


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    h = hashlib.sha256()
    with open(path, "rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def key(name):
    return re.sub(r"[^a-z0-9]+", "_", name.lower()).strip("_")


def inspect(decrypted, cpp):
    require((digest(decrypted), digest(cpp)) == PINS, "reference SHA256 changed")
    with zipfile.ZipFile(decrypted) as z, zipfile.ZipFile(cpp) as c:
        require(len(z.namelist()) == len(set(z.namelist())) == 185, "decrypted entries")
        require(len(c.namelist()) == len(set(c.namelist())) == 316, "C++ entries")
        def read(name):
            return z.read("decrypted files/" + name)
        descriptors = json.loads(read("re/modules_real.json"))
        by_addr = {r["addr"]: r for r in descriptors}
        names = {key(r["name"]): r for r in descriptors}
        require(len(descriptors) == len(by_addr) == len(names) == 67, "descriptor uniqueness")
        data, rdata, text = (read("slinky2.decrypted/" + f + ".bin") for f in ("data", "rdata", "text"))
        # Locate maximal contiguous descriptor-pointer runs, independently of
        # the historical choice of 66 versus 67. Never emit pointer values.
        runs, run = [], []
        for (pointer,) in struct.iter_unpack("<Q", rdata[:len(rdata)//8*8]):
            if pointer - BASE in by_addr:
                run.append(pointer - BASE)
            else:
                if len(run) >= 4:
                    runs.append(run)
                run = []
        if len(run) >= 4:
            runs.append(run)
        require(bool(runs), "registration table absent")
        active = max(runs, key=len)
        require(len(active) == len(set(active)) == 66 and sum(len(r)==66 for r in runs)==1, "ambiguous registration table")
        active_names = {key(by_addr[a]["name"]) for a in active}
        require(set(names)-active_names == {"chest_stealer"}, "unexpected registration difference")
        histogram = [0]*27
        priorities = []
        modules = {}
        for address in active:
            record = by_addr[address]
            off = address - 0x2e5000
            require(0 <= off <= len(data)-0x200, "descriptor bounds")
            category, = struct.unpack_from("<I", data, off+0x10)
            enabled = data[off+0x14]
            gui, = struct.unpack_from("<Q", data, off+0x1f8)
            require(category == record["cat"] and 0 <= category < 5 and enabled == record["en"] == 0, "descriptor metadata mismatch")
            require(gui == (BASE+record["cb"] if record["cb"] else 0), "GUI callback mismatch")
            subscriptions = 0
            for event in range(27):
                callback, priority = struct.unpack_from("<Qi", data, off+0x48+16*event)
                if callback:
                    require(BASE+0x1000 <= callback < BASE+0x1000+len(text), "callback not in text")
                    require(-10000 <= priority <= 10000, "priority bounds")
                    histogram[event] += 1
                    priorities.append(priority)
                    subscriptions += 1
            modules[key(record["name"])] = {"category": CATEGORIES[category], "gui_callback": bool(gui), "subscriptions": subscriptions}
        require(tuple(histogram)==HISTOGRAM and min(priorities)==-10000 and max(priorities)==10000, "subscription histogram drift")
        category_counts = collections.Counter(m["category"] for m in modules.values())
        require(tuple(category_counts[c] for c in CATEGORIES)==(17,10,15,7,17), "category drift")
        legacy = {key(r["name"]) for r in json.loads(read("slinky-modules.json"))}
        generated = {p.rsplit("/",1)[-1][4:-4]: v for p,v in json.loads(read("src/gen/index.json")).items() if p.startswith("src/gen/mod_") and p.endswith(".cpp")}
        logic = {Path(p).stem for p in c.namelist() if re.fullmatch(r"OpenSlinky-main/src/logic/(combat|move|visual|block|utility)/[^/]+\.cpp", p)}
        manager = c.read("OpenSlinky-main/src/engine/jni/module_manager.cpp").decode()
        manager_count = int(re.search(r"manager->count\s*=\s*(0x[0-9a-fA-F]+|\d+)", manager)[1],0)
        require(manager_count==67 and len(logic)==66 and set(names)-logic=={"no_hurt_cam"} and not logic-set(names), "C++ inventory drift")
        audit_rows = [l.split("|")[1:-1] for l in c.read("OpenSlinky-main/docs/module_audit.md").decode("latin1").splitlines() if l.startswith("|")][2:]
        audit = {}
        for row in audit_rows:
            name = key(row[0].strip())
            require(name not in audit, "duplicate C++ report row")
            settings, registered = map(int,re.findall(r"\d+",row[3]))
            functions, byte_count = map(int,re.findall(r"\d+",row[4]))
            audit[name] = (settings, registered, functions, byte_count)
        require(set(audit)==set(names), "C++ report roster drift")
        for name, entry in modules.items():
            entry["cpp_logic_file"] = name in logic
            entry["generated_primary_file"] = name in generated
            entry["reported_settings"] = audit[name][0]
            entry["reported_logic_functions"] = audit[name][2]
        return {"descriptor_count":67,"registered_table_count":66,"descriptor_only":["chest_stealer"],"category_counts":dict(category_counts),"callback_subscriptions":sum(histogram),"event_histogram":histogram,"gui_callbacks":sum(m["gui_callback"] for m in modules.values()),"priority_range":[min(priorities),max(priorities)],"cpp_manager_count":manager_count,"cpp_logic_files":len(logic),"legacy_missing":sorted(set(names)-legacy),"legacy_extra":sorted(legacy-set(names)),"generated_primary_missing":sorted(set(names)-set(generated)),"cpp_report_settings_all":sum(v[0] for v in audit.values()),"cpp_report_settings_active":sum(audit[n][0] for n in active_names),"cpp_report_functions_all":sum(v[2] for v in audit.values()),"cpp_report_functions_active":sum(audit[n][2] for n in active_names),"modules":modules,"commercial_release_authenticated":False,"runtime_parity_verified":False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("decrypted", type=Path)
    parser.add_argument("cpp", type=Path)
    args = parser.parse_args()
    print(json.dumps(inspect(args.decrypted,args.cpp),indent=2,sort_keys=True))


if __name__ == "__main__":
    main()
