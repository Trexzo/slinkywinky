#!/usr/bin/env python3
"""R206 passive original-client evidence classification, deliberately fail-closed.

Observations may show original vanilla foreground/hover state but NEVER
establish exclusive keyboard/mouse ownership, or permit gameplay input.
This module never sends input, opens windows, or accesses processes.
"""
from __future__ import annotations

from dataclasses import asdict, dataclass
from typing import Any


@dataclass(frozen=True)
class PassiveOriginalReadiness:
    original_render_receipts: bool
    foreground_observed: bool
    foreground_samples: int
    source_hover_observed: bool
    exact_source_hover_samples: int
    os_focus_loss: int
    os_focus_regain: int
    original_click_delivery: str
    input_exclusivity_certified: bool
    interactive_gui_verified: bool
    gameplay_modules_verified: bool

    def json_fields(self) -> dict[str, Any]:
        return asdict(self)


def classify_passive_original(
    evidence: dict[str, Any],
    *,
    foreground: int,
    numeric: int,
    bind: int,
    exact: int,
    source_draws: int,
    os_focus: tuple[int, int, int, int, int, int],
) -> PassiveOriginalReadiness:
    """Strictly classifies one restored R99 source-pinned read-only report.

    The returned flags NEVER authorize original input delivery. Source
    hover can be true if physically aligned original-window samples
    exist, but even that proves neither mouse ownership nor a click.
    """
    if type(evidence) is not dict:
        raise ValueError("R206 original report must be a JSON object")
    if (
        evidence.get("scope") != "R81 original vanilla render receipts only"
        or evidence.get("in_game_menu_verified") is not False
        or evidence.get("visual_game_acceptance") is not False
    ):
        raise ValueError("R206 may only assess the original restored read-only R99 scope")
    stages = evidence.get("stages")
    if type(stages) is not list or len(stages) != 6:
        raise ValueError("R206 requires all six original read-only renderer stages")
    expected = (
        "--render-preflight", "--render-callback=8",
        "--render-context=8", "--render-overlay=8",
        "--render-overlay=72", "--render-retained=72",
    )
    if any(type(stage) is not dict or stage.get("mode") != mode
           for stage, mode in zip(stages, expected)):
        raise ValueError("R206 missing or reordered exact original renderer stages")
    last = stages[-1].get("target_receipts")
    if type(last) is not list or any(type(r) is not str for r in last):
        raise ValueError("R206 invalid restored original renderer receipt list")
    exact_restored = [
        r for r in last
        if r.startswith("R99_RETAINED_MENU_DRAW_PASS frames=64 ")
    ]
    if len(exact_restored) != 1:
        raise ValueError("R206 requires one unambiguous restored menu receipt")
    for key, value in (
        ("foreground", foreground), ("numeric", numeric),
        ("bind", bind), ("exact", exact),
        ("source_draws", source_draws),
    ):
        if type(value) is not int or value < 0:
            raise ValueError(f"R206 invalid observed {key} count")
    if (
        source_draws != 60 or foreground > 63 or
        numeric > foreground or bind > foreground or
        numeric + bind > foreground or exact > numeric or
        exact > source_draws
    ):
        raise ValueError("R206 contradictory source-window or hover counters")
    if (
        type(os_focus) is not tuple or len(os_focus) != 6 or
        any(type(count) is not int or count < 0 for count in os_focus)
    ):
        raise ValueError("R206 requires six exact nonnegative Win32 OS focus counters")
    fg, bg, refused, losses, regains, post_hits = os_focus
    if (
        fg + bg + refused != 63 or fg > foreground or
        losses > bg or losses > fg or regains > losses or
        post_hits > exact or post_hits > fg
    ):
        raise ValueError("R206 OS focus counters contradict source evidence")
    return PassiveOriginalReadiness(
        original_render_receipts=True,
        foreground_observed=foreground > 0,
        foreground_samples=foreground,
        source_hover_observed=exact >= 2 and numeric >= 2,
        exact_source_hover_samples=exact,
        os_focus_loss=losses,
        os_focus_regain=regains,
        original_click_delivery="disabled",
        input_exclusivity_certified=False,
        interactive_gui_verified=False,
        gameplay_modules_verified=False,
    )
