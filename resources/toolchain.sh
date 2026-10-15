#!/usr/bin/env bash
set -eu

message() {
    if [ "${NOX_LANGUAGE:-en}" = 'pt_BR' ]; then
        printf '%s\n' "$2"
    else
        printf '%s\n' "$1"
    fi
}

# The installer supplies these arguments after checking the official website.
tool="$1"
version="$2"
url="$3"
checksum="$4"

temporary=$(mktemp -d)
trap 'rm -rf -- "$temporary"' EXIT

message "Downloading $tool $version..." "Baixando $tool $version..."
curl -fL --proto '=https' --proto-redir '=https' --retry 2 "$url" -o "$temporary/package"
printf '%s  %s\n' "$checksum" "$temporary/package" | sha256sum --check -

# Each installation gets a new folder. The previous version stays intact.
base="$HOME/.local/share/nox"
mkdir -p "$base/toolchains" "$base/bin" "$base/current"
destination=$(mktemp -d "$base/toolchains/$tool-$version-XXXXXX")
message 'Extracting package...' 'Extraindo pacote...'
tar -xf "$temporary/package" --strip-components=1 --no-same-owner -C "$destination"

if [ "$tool" = 'node' ]; then
    executables='node npm npx'
    "$destination/bin/node" --version
    PATH="$destination/bin:$PATH" "$destination/bin/npm" --version
else
    executables='go gofmt'
    "$destination/bin/go" version
fi
for program in $executables; do
    test -x "$destination/bin/$program"
done

# Switch the link only after extraction and verification finish.
ln -sfn "$destination" "$base/current/$tool.next"
mv -Tf "$base/current/$tool.next" "$base/current/$tool"
for program in $executables; do
    ln -sfn "$base/current/$tool/bin/$program" "$base/bin/$program"
done

mkdir -p "$HOME/.config/nox" "$HOME/.config/fish/conf.d"
cat > "$HOME/.config/nox/env.sh" <<'ENV'
case ":$PATH:" in
    *":$HOME/.local/share/nox/bin:"*) ;;
    *) export PATH="$HOME/.local/share/nox/bin:$PATH" ;;
esac
ENV

for profile in .profile .bashrc .zshrc; do
    if ! grep -Fq '.config/nox/env.sh' "$HOME/$profile" 2>/dev/null; then
        printf '\n# Nox\n[ -f "$HOME/.config/nox/env.sh" ] && . "$HOME/.config/nox/env.sh"\n' >> "$HOME/$profile"
    fi
done

cat > "$HOME/.config/fish/conf.d/nox.fish" <<'FISH'
if not contains -- "$HOME/.local/share/nox/bin" $PATH
    set -gx PATH "$HOME/.local/share/nox/bin" $PATH
end
FISH

export PATH="$base/bin:$PATH"
if [ "$tool" = 'node' ]; then
    node --version
    npm --version
else
    go version
fi
message 'Done. Open a new terminal to use the installed version.' 'Pronto. Abra um novo terminal para usar a versão instalada.'
