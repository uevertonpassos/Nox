"""Render the canonical SVG for desktop packaging and the project website.

One-time tooling: python -m pip install resvg-py pillow
Run from any directory: python scripts/generate-icons.py
The app itself does not need Python or these packages.
"""
from io import BytesIO
from pathlib import Path
import shutil

from PIL import Image
import resvg_py

root = Path(__file__).resolve().parents[1]
source = root / "resources" / "nox.svg"
png = resvg_py.svg_to_bytes(svg_string=source.read_text(encoding="utf-8"), width=1024, height=1024)
image = Image.open(BytesIO(png)).convert("RGBA")
image.save(root / "resources" / "nox.ico", sizes=[(n, n) for n in (16, 24, 32, 48, 64, 128, 256)])
for size in (48, 128, 256, 512):
    folder = root / "resources" / "icons" / f"{size}x{size}"
    folder.mkdir(parents=True, exist_ok=True)
    image.resize((size, size), Image.Resampling.LANCZOS).save(folder / "nox.png")
assets = root / "docs" / "assets"
assets.mkdir(parents=True, exist_ok=True)
shutil.copyfile(source, assets / "nox.svg")
image.resize((32, 32), Image.Resampling.LANCZOS).save(assets / "favicon.png")
print("Generated Windows ICO, Linux PNG icons, and website logo/favicon.")
