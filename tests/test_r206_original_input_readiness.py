#!/usr/bin/env python3
"""R206 negative controls for read-only original-client acceptance scope.

Pure synthetic report fixtures; these DO NOT launch Minecraft or
claim exclusive physical keyboard/mouse delivery.
"""
from __future__ import annotations

import copy
import json
from r206_original_input_readiness import classify_passive_original


def report() -> dict:
    modes = (
        "--render-preflight", "--render-callback=8",
        "--render-context=8", "--render-overlay=8",
        "--render-overlay=72", "--render-retained=72",
    )
    return {
        "scope": "R81 original vanilla render receipts only",
        "visual_game_acceptance": False,
        "in_game_menu_verified": False,
        "stages": [
            {
                "mode": mode,
                "target_receipts": (
                    ["R99_RETAINED_MENU_DRAW_PASS frames=64 actions=4 tab=1 focus=17"]
                    if i == 5 else
                    ["R81_ORIGINAL_RENDER_ADMISSION_RELEASED_PASS"]
                ),
            }
            for i, mode in enumerate(modes)
        ],
    }


def classify(src=None, **updates):
    args = dict(
        evidence=report() if src is None else src,
        foreground=51,
        numeric=0,
        bind=0,
        exact=0,
        source_draws=60,
        os_focus=(51, 12, 0, 0, 0, 0),
    )
    args.update(updates)
    return classify_passive_original(**args)


def refuse(src=None, **updates):
    try:
        classify(src, **updates)
    except ValueError:
        return
    raise AssertionError("R206 falsely upgraded contradictory or forged original evidence")


def main():
    focus = classify()
    assert focus.original_render_receipts
    assert focus.foreground_observed and focus.foreground_samples == 51
    assert not focus.source_hover_observed
    assert focus.exact_source_hover_samples == 0
    assert focus.original_click_delivery == "disabled"
    assert not focus.input_exclusivity_certified
    assert not focus.interactive_gui_verified
    assert not focus.gameplay_modules_verified
    assert json.loads(json.dumps(focus.json_fields())) == focus.json_fields()

    background = classify(foreground=0, os_focus=(0,63,0,0,0,0))
    assert not background.foreground_observed and not background.source_hover_observed
    hover = classify(
        foreground=4, numeric=3, exact=2,
        os_focus=(4,59,0,1,1,1))
    assert hover.source_hover_observed
    assert hover.foreground_observed
    assert not hover.input_exclusivity_certified
    assert not hover.interactive_gui_verified
    assert not hover.gameplay_modules_verified

    # Existing R181/R183 independent gates can admit observed cursor
    # geometry and OS restoration. Neither yields game-input ownership.
    refuse(foreground=0, numeric=1, os_focus=(0,63,0,0,0,0))
    refuse(foreground=3, numeric=4, os_focus=(3,60,0,0,0,0))
    refuse(foreground=3, numeric=2, bind=2, os_focus=(3,60,0,0,0,0))
    refuse(foreground=3, numeric=2, exact=3, os_focus=(3,60,0,0,0,0))
    refuse(foreground=64, os_focus=(63,0,0,0,0,0))
    refuse(source_draws=59)
    refuse(source_draws=True)
    refuse(foreground=True)
    refuse(numeric=-1)
    refuse(exact=-1)
    refuse(os_focus=[51,12,0,0,0,0])
    refuse(os_focus=(51,12,0,0,0))
    refuse(os_focus=(51,13,0,0,0,0))
    refuse(os_focus=(51,12,0,0,1,0))
    refuse(os_focus=(51,12,0,0,0,1))
    refuse(os_focus=(51,12,0,13,0,0))
    refuse(os_focus=(52,11,0,0,0,0))
    refuse(os_focus=(True,12,0,0,0,0))
    refuse(os_focus=(51,12,0,0,0,-1))

    missing = report()
    missing["stages"].pop()
    refuse(missing)
    wrong_order = report()
    wrong_order["stages"][4], wrong_order["stages"][5] = (
        wrong_order["stages"][5], wrong_order["stages"][4])
    refuse(wrong_order)
    for name, value in (
        ("scope", "operator-controlled gameplay"),
        ("visual_game_acceptance", True),
        ("in_game_menu_verified", True),
        ("in_game_menu_verified", None),
    ):
        tampered = report()
        tampered[name] = value
        refuse(tampered)
    absent = report()
    absent["stages"][-1]["target_receipts"] = []
    refuse(absent)
    duplicated = report()
    duplicated["stages"][-1]["target_receipts"] *= 2
    refuse(duplicated)
    assert classify(report()).input_exclusivity_certified is False
    print("R206_ORIGINAL_READ_ONLY_FOCUS_COUNTS_NOT_INPUT_CERTIFICATION_PASS")
    print("R206_EXACT_SOURCE_HOVER_NOT_CLICK_OR_GAMEPLAY_PROOF_PASS")
    print("R206_FORGED_OR_PROMOTED_ORIGINAL_REPORT_NEGATIVES_PASS")
    print("R206_ORIGINAL_LIVE_CLICK_DISPATCH_REMAINS_DISABLED")


if __name__ == "__main__":
    main()
