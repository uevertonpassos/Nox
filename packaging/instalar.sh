#!/usr/bin/env bash
set -eu

# Execute este arquivo depois de extrair o pacote do Nox.
pasta=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
destino="$HOME/.local"

if ldd "$pasta/bin/nox" | grep -q 'not found'; then
    echo 'Faltam bibliotecas Qt 6. Instale Qt 6 Widgets e Network pela sua distribuição.'
    ldd "$pasta/bin/nox" | grep 'not found'
    exit 1
fi

install -Dm755 "$pasta/bin/nox" "$destino/bin/nox"
install -Dm644 "$pasta/share/icons/hicolor/scalable/apps/nox.svg" "$destino/share/icons/hicolor/scalable/apps/nox.svg"
mkdir -p "$destino/share/doc/nox"
cp -R "$pasta/share/doc/nox/." "$destino/share/doc/nox/"
mkdir -p "$destino/share/applications"
# Usa o caminho completo, mesmo quando ~/.local/bin não estiver no PATH da sessão.
sed "s|^Exec=nox$|Exec=\"$destino/bin/nox\"|" "$pasta/share/applications/io.noxdev.Nox.desktop" > "$destino/share/applications/io.noxdev.Nox.desktop"
echo 'Nox instalado. Abra pelo menu de aplicativos.'
