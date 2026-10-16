#!/usr/bin/env bash
set -eu

# Run this file after extracting the Nox package.
folder=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
destination="$HOME/.local"

if ldd "$folder/bin/nox" | grep -q 'not found'; then
    echo 'Qt 6 libraries are missing. Install Qt 6 Widgets, Network, and Svg through your distribution.'
    ldd "$folder/bin/nox" | grep 'not found'
    exit 1
fi

install -Dm755 "$folder/bin/nox" "$destination/bin/nox"
install -Dm644 "$folder/share/icons/hicolor/scalable/apps/nox.svg" "$destination/share/icons/hicolor/scalable/apps/nox.svg"
for size in 48 128 256 512; do
    install -Dm644 "$folder/share/icons/hicolor/${size}x${size}/apps/nox.png" "$destination/share/icons/hicolor/${size}x${size}/apps/nox.png"
done
mkdir -p "$destination/share/doc/nox"
cp -R "$folder/share/doc/nox/." "$destination/share/doc/nox/"
mkdir -p "$destination/share/applications"
# Use the full path even when ~/.local/bin is not in the session PATH.
sed "s|^Exec=nox$|Exec=\"$destination/bin/nox\"|" "$folder/share/applications/io.noxdev.Nox.desktop" > "$destination/share/applications/io.noxdev.Nox.desktop"
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f -t "$destination/share/icons/hicolor" >/dev/null 2>&1 || true
fi
echo 'Nox installed. Open it from the application menu.'
