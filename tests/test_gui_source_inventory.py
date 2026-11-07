#!/usr/bin/env python3
"""R129: lossless inventory guard, NOT an original-game feature test.

The five category lists and six callback-free descriptors are derived from
the user-provided, *unauthenticated* decrypted files.zip /
re/modules_real.json (archive SHA256
326efa346905370ed1815aba2247d35666008346dd6858262c38070d0f42aea4).
No original DLL, assets, fonts or executable bytes are checked in.
"""
from __future__ import annotations
from collections import Counter
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
CATEGORY = {
    "Combat": "Aim Assist|Auto Block|Auto Clicker|Auto Weapon|Backtrack|Criticals|Hit Select|Hitboxes|Knockback Delay|Knockback Displace|Lag Range|Piercing|Reach|Sprint Reset|Timer Range|Velocity|Weapons",
    "Move": "Fast Accel|Instant Stop|Inv Walk|Keep Sprint|No Jump Delay|No Slow|Null Move|Safe Walk|Sprint|Strafe",
    "Visual": "Anti Debuff|Block ESP|Chams|Chest ESP|Free Look|GUI|Indicators|Item ESP|Mod Overlay|Name Tags|No Hurt Cam|Notifications|Player ESP|Pointers|Trajectories",
    "Block": "Auto Tool|Block In|Bridge Assist|Clutch|Fast Mine|Fast Place|Ghost Hand",
    "Utility": "Anti Bot|Auto Pot|Auto Rod|Auto Soup|Blink|Chest Stealer|Debug|Fake Lag|Friends|Inv Manager|Item Use Fix|No Hit Delay|No Item Release|No Use Delay|Ping Fix|Refill|Teams|Timer",
}
ENGINE_ONLY = {
    "Instant Stop", "No Jump Delay", "Null Move",
    "No Hurt Cam", "Item Use Fix", "Ping Fix",
}
EXPECTED_CATEGORY_COUNTS = {
    "Combat": 17, "Move": 10, "Visual": 15, "Block": 7, "Utility": 18,
}
EXPECTED_GUI_DESCRIPTOR_COUNT = 61
EXPECTED_FIXTURE_WIDGET_COUNT = 385  # R122+ conditioned fixture, NOT native parity


def main() -> None:
    expected = {
        (name, category)
        for category, names in CATEGORY.items()
        for name in names.split("|")
    }
    assert len(expected) == 67, "reference descriptor duplicates/missing"
    assert {
        cat: len(names.split("|")) for cat, names in CATEGORY.items()
    } == EXPECTED_CATEGORY_COUNTS
    catalog_source = (ROOT / "src" / "r115_recovered_catalog.inc").read_text(
        encoding="utf-8"
    )
    actual = re.findall(
        r'\{\s*"([^"]+)"\s*,\s*MenuTab::([A-Za-z]+)\s*\}',
        catalog_source,
    )
    assert len(actual) == 67, f"candidate catalog has {len(actual)} descriptors"
    assert len(set(actual)) == 67, "duplicate catalog descriptor"
    missing = sorted(expected - set(actual))
    unexpected = sorted(set(actual) - expected)
    assert not missing and not unexpected, (
        f"catalog mismatch: missing={missing}, unexpected={unexpected}"
    )

    widget_source = (ROOT / "src" / "recovered_settings.cpp").read_text(
        encoding="utf-8"
    )
    widgets = re.findall(
        r'\{\s*"([^"]+)"\s*,\s*"[^"]*"\s*,\s*"[^"]*"\s*,\s*'
        r'RecoveredWidgetKind::',
        widget_source,
    )
    counts = Counter(widgets)
    assert len(widgets) == EXPECTED_FIXTURE_WIDGET_COUNT, (
        f"fixture changed: {len(widgets)} widgets; re-audit the source"
    )
    expected_gui_names = {n for n, _ in expected} - ENGINE_ONLY
    assert len(expected_gui_names) == EXPECTED_GUI_DESCRIPTOR_COUNT
    assert set(counts) == expected_gui_names, (
        f"missing GUI descriptors={sorted(expected_gui_names - set(counts))}; "
        f"unexpected={sorted(set(counts) - expected_gui_names)}"
    )
    assert all(value > 0 for value in counts.values())
    print(
        "R129_SOURCE_INVENTORY_PASS descriptor_count=67 "
        "gui_descriptors=61 engine_only=6 "
        "fixture_widgets=385 native_widget_callsites_reference=424"
    )
    print(
        "NOTICE: 385 typed fixture widgets are not proven equal to the "
        "424 native W_* callsites. All gameplay module fidelity unverified."
    )


if __name__ == "__main__":
    main()
