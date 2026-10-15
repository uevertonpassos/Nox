#!/usr/bin/env bash
set -eu

# Os argumentos são definidos pelo instalador após consultar o site oficial.
ferramenta="$1"
versao="$2"
url="$3"
checksum="$4"

temporario=$(mktemp -d)
trap 'rm -rf -- "$temporario"' EXIT

echo "Baixando $ferramenta $versao..."
curl -fL --proto '=https' --proto-redir '=https' --retry 2 "$url" -o "$temporario/pacote"
printf '%s  %s\n' "$checksum" "$temporario/pacote" | sha256sum --check -

# Cada instalação recebe uma pasta nova. A versão anterior permanece intacta.
base="$HOME/.local/share/nox"
mkdir -p "$base/toolchains" "$base/bin" "$base/current"
destino=$(mktemp -d "$base/toolchains/$ferramenta-$versao-XXXXXX")
echo 'Extraindo pacote...'
tar -xf "$temporario/pacote" --strip-components=1 --no-same-owner -C "$destino"

if [ "$ferramenta" = 'node' ]; then
    executaveis='node npm npx'
    "$destino/bin/node" --version
    PATH="$destino/bin:$PATH" "$destino/bin/npm" --version
else
    executaveis='go gofmt'
    "$destino/bin/go" version
fi
for programa in $executaveis; do
    test -x "$destino/bin/$programa"
done

# Troca o link somente depois que a extração e a verificação terminaram.
ln -sfn "$destino" "$base/current/$ferramenta.next"
mv -Tf "$base/current/$ferramenta.next" "$base/current/$ferramenta"
for programa in $executaveis; do
    ln -sfn "$base/current/$ferramenta/bin/$programa" "$base/bin/$programa"
done

mkdir -p "$HOME/.config/nox" "$HOME/.config/fish/conf.d"
cat > "$HOME/.config/nox/env.sh" <<'ENV'
case ":$PATH:" in
    *":$HOME/.local/share/nox/bin:"*) ;;
    *) export PATH="$HOME/.local/share/nox/bin:$PATH" ;;
esac
ENV

for perfil in .profile .bashrc .zshrc; do
    if ! grep -Fq '.config/nox/env.sh' "$HOME/$perfil" 2>/dev/null; then
        printf '\n# Nox\n[ -f "$HOME/.config/nox/env.sh" ] && . "$HOME/.config/nox/env.sh"\n' >> "$HOME/$perfil"
    fi
done

cat > "$HOME/.config/fish/conf.d/nox.fish" <<'FISH'
if not contains -- "$HOME/.local/share/nox/bin" $PATH
    set -gx PATH "$HOME/.local/share/nox/bin" $PATH
end
FISH

export PATH="$base/bin:$PATH"
if [ "$ferramenta" = 'node' ]; then
    node --version
    npm --version
else
    go version
fi
echo 'Pronto. Abra um novo terminal para usar a versão instalada.'
