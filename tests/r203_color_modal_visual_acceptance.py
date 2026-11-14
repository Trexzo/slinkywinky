#!/usr/bin/env python3
"""R203: isolated WGL readback comparison; no desktop/game screenshot capture.

This manually invoked visual receipt uses Pillow to convert raw fixture PPMs,
reports changed pixels, and can reject diff spilling outside the modal area.
It does not compare to an authenticated original Slinky screenshot.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path

from PIL import Image, ImageChops


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--before", type=Path, required=True)
    parser.add_argument("--after", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    # Never let Python -O silently disable the fixture's assert checks.
    if not __debug__:
        raise RuntimeError("R203 visual assertions require ordinary Python")

    old = Image.open(args.before).convert("RGB")
    new = Image.open(args.after).convert("RGB")
    assert old.size == new.size == (676, 630), "Wrong native fixture size"
    args.out.mkdir(parents=True, exist_ok=True)
    targets = [
        args.out / "r203-color-popup-original-wgl.png",
        args.out / "r203-color-popup-hsl-wgl.png",
    ]
    for img, target in zip((old, new), targets):
        img.save(target, optimize=True)
        print(
            f"PNG {target.name} bytes={target.stat().st_size} "
            f"sha256={hashlib.sha256(target.read_bytes()).hexdigest()}"
        )
    difference = ImageChops.difference(old, new)
    bbox = difference.getbbox()
    before_pixels = old.load()
    after_pixels = new.load()
    changed = sum(
        before_pixels[x, y] != after_pixels[x, y]
        for y in range(old.height)
        for x in range(old.width)
    )
    print(f"DIFF_CHANGED_PIXELS={changed} DIFF_BBOX={bbox}")
    # Pixel-local analysis at the exact 676x630 fixture viewport.
    # The popup measured from the WGL screenshots spans about
    # (211,211)..(465,419), distinct from surrounding animated cards.
    roi=(211,211,465,419)
    inside=outside=0
    for y in range(old.height):
        for x in range(old.width):
            if before_pixels[x,y]==after_pixels[x,y]:
                continue
            if roi[0]<=x<roi[2] and roi[1]<=y<roi[3]:
                inside+=1
            else:
                outside+=1
    print(f"DIFF_MODAL_PIXELS={inside} DIFF_BACKGROUND_PIXELS={outside}")
    for x, y in [
        (230,252), (235,252), (442,228), (234,288),
        (340,322), (340,361), (225,400),
        (338,315), (338,200), (338,270),
        (225,288), (265,288), (304,288), (340,288),
        (384,288), (425,288),
        (230,324), (442,324), (230,362), (442,362),
        (200,245), (100,100), (10,10), (338,430)
    ]:
        print(
            f"PIXEL ({x},{y}) "
            f"original={before_pixels[x,y]} modified={after_pixels[x,y]}"
        )
    # Strict known RGB acceptance in the fixed 676x630 native fixture.
    # We assert only the popup's internal evidence, NOT that the page
    # behind it stays pixel-identical during CSS-like fading/animation.
    source_swatch=before_pixels[230,252]
    changed_swatch=after_pixels[230,252]
    assert source_swatch==(0x8B,0x7C,0xF6), (
        f"Source 8B7CF6 modal swatch drift: {source_swatch}"
    )
    assert changed_swatch != source_swatch, (
        "HSL drag failed to change the actual painted swatch"
    )
    assert changed_swatch[1] > source_swatch[1]+20, (
        "Hue change not visible as the expected greener blue"
    )
    assert before_pixels[225,400]==after_pixels[225,400]==(0,0,0), (
        "Source-initialized recent black swatch not preserved"
    )
    assert before_pixels[442,228]==after_pixels[442,228], (
        "Source-style close cross changed during HSL drag"
    )
    assert before_pixels[10,10]==after_pixels[10,10], (
        "Isolated WGL framebuffer clear/background changed"
    )
    assert inside>500 and changed>inside, (
        f"Insufficient actual color-modal visual change: {inside}"
    )
    hue_ramp=[before_pixels[x,288] for x in (225,265,304,340,425)]
    assert len(set(hue_ramp))==5, (
        "Source six-sector hue ramp is not visibly interpolated"
    )
    assert (min(c[0] for c in hue_ramp)<130 and
            max(c[0] for c in hue_ramp)>235 and
            min(c[1] for c in hue_ramp)<135 and
            max(c[1] for c in hue_ramp)>235), (
        f"Hue bar lost full-spectrum RGB coverage: {hue_ramp}"
    )
    assert before_pixels[230,324]!=before_pixels[442,324], (
        "Saturation bar no longer changes across width"
    )
    assert before_pixels[230,362][0]<20 and (
        before_pixels[442,362][0]>230
    ), "Lightness bar lost black-to-white ramp"
    print("R203_SOURCE_HUE_SATURATION_LIGHTNESS_PIXELS_PASS")
    print("R203_WGL_MODAL_SWATCH_HSL_RECENT_AND_CLOSE_PIXEL_PASS")
    print("R203_BACKGROUND_ANIMATION_DIFF_NOT_SOURCE_PARITY_PROOF")


if __name__ == "__main__":
    main()
