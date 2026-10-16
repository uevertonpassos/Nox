"""Convert real Qt captures to lossless WebP assets for GitHub Pages.

Requires Pillow. Run window_test with NOX_SCREENSHOT_DIR first.
Usage: python scripts/prepare-screenshots.py build/screenshots
"""
import sys
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[1]
source = Path(sys.argv[1] if len(sys.argv) > 1 else root / "build" / "screenshots")
for theme in ("mint", "ocean", "violet", "ember", "daylight"):
    with Image.open(source / f"nox-{theme}.png") as screenshot:
        output = root / "docs" / "assets" / f"nox-{theme}.webp"
        screenshot.convert("RGB").save(output, "WEBP", lossless=True, method=6)
        print(f"{output.name}: {screenshot.width}x{screenshot.height}, {output.stat().st_size:,} bytes")
