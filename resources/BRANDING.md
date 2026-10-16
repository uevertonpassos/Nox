# Nox identity

`nox.svg` is the canonical, editable logo: a folded N and a small spark on a mint tile. It was drawn for Nox as SVG geometry, so the same mark stays crisp in the app, Windows taskbar, Linux application menu, and project website.

- Primary color: `#b7ed8b` (mint).
- Ink: `#172510`.
- The mark has a transparent outer margin and works on light or dark backgrounds.
- The app wordmark uses the platform's UI typeface; there is no additional font dependency.

Regenerate the Windows multi-resolution ICO, Linux PNG sizes, and website favicon after editing the SVG:

```sh
python -m pip install pillow resvg-py
python scripts/generate-icons.py
```

These are development tools only. The packaged application has no Python dependency.

Theme tokens live in `src/theme.cpp`. `resources/style.qss` uses those tokens, and the Qt palette keeps native controls and dialogs consistent with the selected theme.
