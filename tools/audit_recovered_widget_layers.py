#!/usr/bin/env python3
"""Compare three candidate GUI layers without executing the decrypted archive.

Usage:
  python tools/audit_recovered_widget_layers.py \
      --archive "/path/to/decrypted files.zip"

Each layer has different semantics: HTML display constructors, generated
native registration calls, and this repository's conditional display
callsites. Treat all archive material as an unauthenticated candidate.
"""
import argparse
import collections
import json
import pathlib
import re
import sys
import zipfile

KINDS = ("Boolean", "Slider", "Dropdown", "Range",
         "MultiSelect", "Color", "Text", "Button")
JS_KIND = dict(zip("CSDRMKTB", KINDS))
JS_MODULE = re.compile(r"\['([^']+)','([^']*)',\s*\[")
JS_CONTROL = re.compile(r"(?<![A-Za-z0-9_])([CSDRMKTB])\(")
CPP_WIDGET = re.compile(
    r'^\s*\{"([^"]+)",[^\n]*?RecoveredWidgetKind::(\w+)', re.M)
NATIVE_KIND = re.compile(r"\b(R_[A-Za-z]+)\s*\(")
EXPECTED_DIFF_MODULES = {"Aim Assist", "Piercing"}
EXPECTED_HTML_TOTAL = 378
EXPECTED_FIXTURE_TOTAL = 385


def audit(archive: pathlib.Path, widget_file: pathlib.Path):
    with zipfile.ZipFile(archive) as zf:
        def read(suffix):
            files = [x for x in zf.namelist() if x.endswith(suffix)]
            if len(files) != 1:
                raise ValueError("ambiguous or missing " + suffix)
            info = zf.getinfo(files[0])
            if info.file_size > 4_000_000:
                raise ValueError("candidate file too large: " + suffix)
            return zf.read(info).decode("utf-8")
        html_widgets = read("/re/mods_block.js")
        aim_native = read("/src/gen/mod_aim_assist.cpp")
        piercing_native = read("/src/gen/mod_piercing.cpp")
    modules = list(JS_MODULE.finditer(html_widgets))
    if len(modules) != 67:
        raise ValueError("expected 67 HTML-reconstructed module descriptors")
    html = {}
    for index, found in enumerate(modules):
        name = found.group(1)
        if name in html:
            raise ValueError("duplicate HTML module: " + name)
        end = modules[index+1].start() if index+1<len(modules) else len(html_widgets)
        body = html_widgets[found.end():end]
        counts = collections.Counter(JS_KIND[k] for k in JS_CONTROL.findall(body))
        html[name] = counts
    fixture = collections.defaultdict(collections.Counter)
    for name, kind in CPP_WIDGET.findall(widget_file.read_text(encoding="utf-8")):
        if kind not in KINDS:
            raise ValueError("unknown fixture kind " + kind)
        fixture[name][kind] += 1
    if any(name not in html for name in fixture):
        raise ValueError("fixture has module absent from candidate HTML")
    total_html = sum(sum(v.values()) for v in html.values())
    total_fixture = sum(sum(v.values()) for v in fixture.values())
    differences = {}
    for name, html_counts in html.items():
        actual = fixture.get(name, {})
        if any(html_counts.get(k, 0)!=actual.get(k, 0) for k in KINDS):
            differences[name] = {
                "html": dict(html_counts),
                "fixture": dict(actual),
            }
    aim = collections.Counter(NATIVE_KIND.findall(aim_native))
    piercing = collections.Counter(NATIVE_KIND.findall(piercing_native))
    correct_native = (
        all(aim[k]==v for k,v in
            {"R_bool":11,"R_num":6,"R_enum":3,"R_rng":2}.items()) and
        all(piercing[k]==v for k,v in
            {"R_bool":7,"R_num":0,"R_enum":1,"R_rng":0}.items()))
    good = (len(fixture)==61 and len(html)==67 and
            total_html==EXPECTED_HTML_TOTAL and
            total_fixture==EXPECTED_FIXTURE_TOTAL and
            set(differences)==EXPECTED_DIFF_MODULES and
            correct_native)
    return {
        "html_module_count":len(html),
        "fixture_module_count":len(fixture),
        "html_widget_constructors":total_html,
        "fixture_display_callsites":total_fixture,
        "different_modules":differences,
        "native_aim_registration":dict(aim),
        "native_piercing_registration":dict(piercing),
        "expected_layer_divergence":good,
        "interpretation":"GUI callsites are not one-to-one native settings; do not flatten aliases",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive",required=True,type=pathlib.Path)
    parser.add_argument(
        "--widgets",type=pathlib.Path,
        default=pathlib.Path(__file__).resolve().parents[1]/
                "src/recovered_settings.cpp")
    args=parser.parse_args()
    try:
        result=audit(args.archive,args.widgets)
    except (OSError,ValueError,zipfile.BadZipFile,UnicodeError) as exc:
        print("AUDIT_ERROR:",exc,file=sys.stderr)
        return 2
    print(json.dumps(result,indent=2,ensure_ascii=False))
    return 0 if result["expected_layer_divergence"] else 1


if __name__=="__main__":
    raise SystemExit(main())
