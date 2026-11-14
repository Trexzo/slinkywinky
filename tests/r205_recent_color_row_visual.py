#!/usr/bin/env python3
"""R205: first-party WGL recent-color row pixel receipt, not original Slinky parity.

Requires explicit GL_BACK captures from the same 676x630 Windows fixture.
The historical R204 PNG is a versioned, first-party baseline, not vendor art.
Pillow is needed only for this opt-in developer-side comparison.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
from PIL import Image, ImageChops


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--old", required=True, type=Path)
    ap.add_argument("--new", required=True, type=Path)
    ap.add_argument("--new-hsl", required=True, type=Path)
    ap.add_argument("--out", required=True, type=Path)
    args = ap.parse_args()
    if not __debug__:
        raise RuntimeError("R205 visual checks cannot run with Python -O")
    old = Image.open(args.old).convert("RGB")
    new = Image.open(args.new).convert("RGB")
    after_hsl = Image.open(args.new_hsl).convert("RGB")
    if old.size != (676, 630) or new.size != old.size or after_hsl.size != old.size:
        raise AssertionError("R205 requires identical 676x630 WGL snapshots")
    args.out.mkdir(parents=True, exist_ok=True)
    for im, target in [
        (new, args.out/"r205-color-popup-recent-spacing-wgl.png"),
        (after_hsl, args.out/"r205-color-popup-recent-spacing-hsl-wgl.png"),
    ]:
        im.save(target, optimize=True)
        print(f"PNG {target.name} bytes={target.stat().st_size} "
              f"sha256={hashlib.sha256(target.read_bytes()).hexdigest()}")
    box = ImageChops.difference(old, new).getbbox()
    o = old.load()
    n = new.load()
    total = inside = 0
    # The source padding/stride change must affect only this 25px row.
    # The coordinates are from the owner-checked 676x630 WGL fixture.
    for y in range(old.height):
        for x in range(old.width):
            if o[x, y] == n[x, y]:
                continue
            total += 1
            if 218 <= x < 458 and 378 <= y < 414:
                inside += 1
    print(f"R205_DIFF_BBOX={box} CHANGED={total} IN_ROW={inside} OUTSIDE={total-inside}")
    if box != (220,382,456,409) or total != 1210 or inside != total:
        raise AssertionError("R205 recent-row visual delta escapes source geometry")
    expected = {
        (220,400): ((20,20,20),(0,0,0)),
        (222,400): ((20,20,20),(0,0,0)),
        (225,400): ((0,0,0),(0,0,0)),
        (246,400): ((0,0,0),(20,20,20)),
        (252,400): ((20,20,20),(0,0,0)),
        (454,400): ((20,20,20),(0,0,0)),
        (230,252): ((139,124,246),(139,124,246)),
        (340,322): ((158,149,221),(158,149,221)),
        (442,228): ((153,153,153),(153,153,153)),
        (100,100): ((26,97,176),(26,97,176)),
    }
    for (x,y), (previous,current) in expected.items():
        if o[x,y] != previous or n[x,y] != current:
            raise AssertionError(
                f"R205 sample {(x,y)} expected {previous}->{current}, "
                f"got {o[x,y]}->{n[x,y]}")
    print("R205_SOURCE_RECENT_PADDING_AND_8PX_SPACING_PIXELS_PASS")
    print("R205_PIXEL_DELTA_CONFINED_TO_COLOR_HISTORY_ROW_PASS")
    print("R205_FIRST_PARTY_WGL_ONLY_NOT_VENDOR_PIXEL_PARITY")


if __name__ == "__main__":
    main()
