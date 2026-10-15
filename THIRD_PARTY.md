# Libraries and images

Nox uses Qt Core, GUI, Widgets, Network, and SVG. These modules are linked dynamically. The Windows installer and AppImage include the libraries needed to run the app.

Qt is available under LGPLv3. The LGPLv3 and GPLv3 texts are in `resources/licenses` and are included in the packages. The LGPL allows replacing the libraries with compatible modified versions and reverse engineering to debug those modifications.

Sources used in the builds:

- [Qt 6.8.3](https://download.qt.io/archive/qt/6.8/6.8.3/) on Windows.
- [qt6-base](https://packages.ubuntu.com/source/jammy/qt6-base), [qt6-svg](https://packages.ubuntu.com/source/jammy/qt6-svg), and [qt6-wayland](https://packages.ubuntu.com/source/jammy/qt6-wayland) from Ubuntu 22.04 in the AppImage.
- [Qt licensing information](https://doc.qt.io/qt-6/licensing.html).

Logos identify the tools installed by the app. Sources and credits are in [resources/logos/SOURCES.md](resources/logos/SOURCES.md). The C and C++ icons come from Devicon, under the MIT license.
