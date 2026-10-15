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
mkdir -p "$destination/share/doc/nox"
cp -R "$folder/share/doc/nox/." "$destination/share/doc/nox/"
mkdir -p "$destination/share/applications"
# Use the full path even when ~/.local/bin is not in the session PATH.
sed "s|^Exec=nox$|Exec=\"$destination/bin/nox\"|" "$folder/share/applications/io.noxdev.Nox.desktop" > "$destination/share/applications/io.noxdev.Nox.desktop"
echo 'Nox installed. Open it from the application menu.'
