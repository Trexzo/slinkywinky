#!/usr/bin/env python3
"""Verify pinned source identities and coverage accounting without running reference code."""
import argparse
import collections
import hashlib
import json
import re
import zipfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
LEDGER=ROOT/"docs/R315_REMAINING_REFERENCE_LEDGER.json"
def require(ok,message):
    if not ok:
        raise ValueError(message)

def audit(ledger, matrix, archive=None):
    rows={}
    for line in matrix.splitlines():
        m=re.match(r"^\| (block|combat|move|utility|visual) \| \x60([a-z0-9_]+)\x60 \| ([^|]+) \|",line)
        if m:
            category,name,status=m.groups()
            require(name not in rows,"duplicate matrix row")
            rows[name]=(category,status.strip())
    require(len(rows)==67 and "chest_stealer" in rows,"descriptor count")
    active={k:v for k,v in rows.items() if k!="chest_stealer"}
    require(collections.Counter(v[0] for v in active.values())==
            dict(combat=17,move=10,visual=15,block=7,utility=17),"category drift")
    require(ledger["schema"]==1 and ledger["runtime_parity_verified"] is False,"ledger schema")
    entries=ledger["modules"]
    require(len(entries)==11 and len({x["name"] for x in entries})==11,"ledger roster")
    for entry in entries:
        name=entry["name"]
        require(name in active and active[name][0]==entry["category"],"ledger category mismatch")
        require(re.fullmatch(r"[a-f0-9]{64}",entry["sha256"]) is not None,"source hash format")
        for field in ("settings_reported","registered_subscriptions","functions_reported"):
            require(type(entry[field]) is int and entry[field]>=0,"bad count")
    totals=tuple(sum(x[k] for x in entries) for k in ("settings_reported","registered_subscriptions","functions_reported"))
    require(totals==(79,40,42),"ledger totals")
    missing=sorted(k for k,v in active.items() if v[1]=="Not implemented")
    require(set(missing)<={x["name"] for x in entries},"new gap outside historical ledger")
    if archive is not None:
        require(hashlib.sha256(archive.read_bytes()).hexdigest()==ledger["reference_archive_sha256"],"archive hash mismatch")
        with zipfile.ZipFile(archive) as z:
            require(len(z.namelist())==len(set(z.namelist())),"duplicate zip members")
            for entry in entries:
                path="OpenSlinky-main/src/logic/{category}/{name}.cpp".format(**entry)
                require(hashlib.sha256(z.read(path)).hexdigest()==entry["sha256"],"source identity mismatch")
    counts=collections.Counter("offline" if v[1].startswith("Offline ") else
         "experimental" if v[1]=="Experimental decision model (offline)" else
         "missing" if v[1]=="Not implemented" else "unknown" for v in active.values())
    require(not counts["unknown"],"unknown state")
    return {"original_table":66,"descriptor_only":["chest_stealer"],"coverage":dict(counts),
            "missing":missing,"historical_gap_totals":list(totals),
            "archive_verified_this_run":archive is not None,"runtime_parity_verified":False}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--archive",type=Path)
    args=p.parse_args()
    print(json.dumps(audit(json.loads(LEDGER.read_text(encoding="utf-8")),
        (ROOT/"docs/MODULE_MATRIX.md").read_text(encoding="utf-8"),args.archive),indent=2))
if __name__=="__main__":
    main()
