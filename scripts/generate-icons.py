#!/usr/bin/env python3
# Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# This file is part of PvZ-Portable.
#
# PvZ-Portable is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# PvZ-Portable is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with PvZ-Portable. If not, see <https://www.gnu.org/licenses/>.

"""
PvZ-Portable Icon Generator

Derives the raster platform icons from the single vector source icon.svg
(located at the repository root). Platforms that support SVG (Linux desktop,
README) use icon.svg directly and are not handled here.

Requirements: rsvg-convert (librsvg) and Pillow.

Usage:
    python3 scripts/generate-icons.py
"""

import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SVG = ROOT / "icon.svg"

# Fraction of the square canvas that the logo's longer side may occupy.
FIT = 0.94

# (output path relative to repo root, canvas size, background or None for
# transparent, force RGB output)
TARGETS = [
    ("android/app/src/main/res/mipmap-xxxhdpi/ic_launcher.png", 512, None, False),
    ("ios/Assets.xcassets/AppIcon.appiconset/AppIcon.png", 1024, (255, 255, 255), True),
    ("icon-switch.jpg", 256, (0, 0, 0), True),
    ("icon-3ds.png", 48, (0, 0, 0), True),
]

ICO_SIZES = [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]


def render_logo() -> Image.Image:
    """Render icon.svg at high resolution and crop to the content bounds."""
    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
        tmp_path = tmp.name
    subprocess.run(
        ["rsvg-convert", "-w", "2600", "-h", "1800", str(SVG), "-o", tmp_path],
        check=True,
    )
    img = Image.open(tmp_path).convert("RGBA")
    Path(tmp_path).unlink()
    bbox = img.getchannel("A").getbbox()
    if bbox is None:
        sys.exit("error: icon.svg rendered to a fully transparent image")
    return img.crop(bbox)


def compose(logo: Image.Image, size: int, background) -> Image.Image:
    """Scale the logo to fit a square canvas and center it."""
    scale = min(size * FIT / logo.width, size * FIT / logo.height)
    w = max(1, round(logo.width * scale))
    h = max(1, round(logo.height * scale))
    scaled = logo.resize((w, h), Image.LANCZOS)
    if background is None:
        canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    else:
        canvas = Image.new("RGBA", (size, size), background + (255,))
    # Round the offsets toward the center so the logo stays optically centered.
    canvas.alpha_composite(scaled, ((size - w) // 2, (size - h) // 2))
    return canvas


def main() -> None:
    logo = render_logo()
    print(f"rendered logo content: {logo.width}x{logo.height}")

    for rel_path, size, background, force_rgb in TARGETS:
        out = ROOT / rel_path
        canvas = compose(logo, size, background)
        if force_rgb:
            canvas = canvas.convert("RGB")
        if out.suffix == ".jpg":
            canvas.save(out, quality=95)
        else:
            canvas.save(out)
        print(f"wrote {rel_path} ({size}x{size})")

    master = compose(logo, 1024, None)
    master.save(ROOT / "icon.ico", sizes=ICO_SIZES)
    print(f"wrote icon.ico ({', '.join(f'{w}x{h}' for w, h in ICO_SIZES)})")


if __name__ == "__main__":
    main()
