#!/usr/bin/env python3
"""R204 screenshot change audit for own disposable WGL color modal.

These are native first-party rendered pixels, not desktop/game capture.
R203 reference is our prior renderer, NOT an authenticated Slinky image.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path

from PIL import Image, ImageChops


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--old", type=Path, required=True)
    parser.add_argument("--new", type=Path, required=True)
    parser.add_argument("--new-hsl", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    if not __debug__:
        raise RuntimeError("R204 visual checks must not be disabled with -O")
    before=Image.open(args.old).convert("RGB")
    after=Image.open(args.new).convert("RGB")
    after_hsl=Image.open(args.new_hsl).convert("RGB")
    if any(im.size != (676,630) for im in (before,after,after_hsl)):
        raise RuntimeError("R204 expected exact 676x630 source fixture")
    args.out.mkdir(parents=True,exist_ok=True)
    for image,path in [
        (after,args.out/"r204-color-popup-rounded-wgl.png"),
        (after_hsl,args.out/"r204-color-popup-hsl-wgl.png"),
    ]:
        image.save(path,optimize=True)
        print("PNG",path.name,"bytes",path.stat().st_size,
              "sha256",hashlib.sha256(path.read_bytes()).hexdigest())
    dif=ImageChops.difference(before,after)
    print("R203_R204_DIFF_BBOX",dif.getbbox())
    a=before.load();b=after.load()
    count=0
    for y in range(630):
        for x in range(676):
            count+=(a[x,y]!=b[x,y])
    print("R203_R204_CHANGED_PIXELS",count)
    # R204: this fixture's only newly painted geometry is the 5px
    # rounded HSL ends plus the S/L diagonal gradient approximation.
    # No background module, title, color-swatch, or recent history
    # pixel may change relative to our already versioned R203 frame.
    if not (250 <= count <= 6000):
        raise AssertionError(f"unexpected gradient pixel delta: {count}")
    bounds=dif.getbbox()
    if not bounds or not (
        220 <= bounds[0] <= 230 and
        280 <= bounds[1] <= 286 and
        445 <= bounds[2] <= 457 and
        364 <= bounds[3] <= 372
    ):
        raise AssertionError(f"gradient diff escaped source bar rects: {bounds}")
    expected={
        (225,283):((246,127,124),(20,20,20)),
        (450,283):((246,124,127),(20,20,20)),
        (225,320):((185,185,185),(20,20,20)),
        (225,366):((1,1,1),(20,20,20)),
        (230,252):((139,124,246),(139,124,246)),
        (442,228):((153,153,153),(153,153,153)),
        (225,400):((0,0,0),(0,0,0)),
        (100,100):((26,97,176),(26,97,176)),
    }
    for (x,y),(was,now) in expected.items():
        if a[x,y] != was or b[x,y] != now:
            raise AssertionError(
                f"R204 fixed WGL pixel {(x,y)} drift: "
                f"expected {was}->{now}, got {a[x,y]}->{b[x,y]}"
            )
    print("R204_ROUNDED_HSL_EDGES_EXACT_PIXEL_PASS")
    print("R204_ONLY_HSL_GRADIENT_REGION_CHANGED_PASS")
    coords=[
        (225,283),(226,283),(227,283),(228,283),(229,283),
        (230,283),(231,283),(234,283),(234,288),(225,286),
        (225,288),(225,290),(225,294),(225,295),(230,294),
        (444,283),(449,283),(450,283),(451,283),
        (225,318),(225,320),(225,322),(225,324),(230,322),
        (225,355),(225,358),(225,361),(225,362),(225,366),
        (230,252),(340,322),(442,228),(225,400),(100,100),
    ]
    for x,y in coords:
        print("SAMPLE",x,y,"R203",a[x,y],"R204",b[x,y])


if __name__ == "__main__":
    main()
