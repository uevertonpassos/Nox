# Nox

Um aplicativo para instalar as ferramentas que uso para programar, sem precisar procurar o comando de cada uma.

Feito em C++ com Qt 6. Tem versões para Linux e Windows.

## Download

- [Windows x64 — instalador .exe](https://github.com/uevertonpassos/Nox/releases/latest/download/Nox-Windows-x64.exe)
- [Linux x86_64 — AppImage](https://github.com/uevertonpassos/Nox/releases/latest/download/Nox-Linux-x86_64.AppImage)
- [Linux x86_64 — pacote .tar.gz](https://github.com/uevertonpassos/Nox/releases/latest/download/Nox-Linux-x86_64.tar.gz)

[Ver todas as versões](https://github.com/uevertonpassos/Nox/releases)

No Windows, abra o instalador. O Nox usa o WinGet para instalar as ferramentas.

No Linux, dê permissão ao AppImage e execute:

```bash
chmod +x Nox-Linux-x86_64.AppImage
./Nox-Linux-x86_64.AppImage
```

Se aparecer um erro sobre FUSE, use:

```bash
./Nox-Linux-x86_64.AppImage --appimage-extract-and-run
```

O AppImage é compilado no Ubuntu 22.04 e precisa de glibc 2.35 ou mais recente. O pacote `.tar.gz` usa o Qt 6 instalado no sistema: extraia e execute `bash instalar.sh`.

## O que instala

- Node.js LTS
- npm, junto com o Node.js LTS
- Go estável
- Zed
- Docker Desktop ou Docker Engine
- Compiladores de C e C++

Go e npm não têm um canal LTS próprio. Para npm, uso a versão que acompanha Node.js LTS.

No Linux, há comandos para Debian/Ubuntu, Fedora, Arch/CachyOS e openSUSE. No Windows, Docker usa a versão Desktop. No Arch/CachyOS, a opção disponível é Docker Engine.

## Como usar

Busque uma ferramenta, clique em instalar e confira os comandos antes de confirmar. O resultado aparece na parte de baixo da janela.

“Detectado” significa que o executável foi encontrado. Isso não confirma a versão nem, no caso do Docker, se o serviço está rodando.

Alguns detalhes:

- É preciso ter internet para baixar as ferramentas.
- No Linux, abra o Nox sem `sudo`. A senha será pedida pelo sistema quando necessária.
- Node.js e Go ficam em `~/.local/share/nox`. Abra um terminal novo depois de instalar.
- No Arch/CachyOS, a instalação de pacotes também atualiza o sistema com `pacman -Syu`.
- Docker Desktop precisa de virtualização. No Linux, exige KVM; no Windows, WSL 2 ou Hyper-V.
- NixOS, Alpine e a instalação de pacotes em sistemas imutáveis ainda não são atendidos.
- O instalador Windows não tem assinatura digital.

## Compilar

Precisa de C++17, CMake 3.22+, Ninja e Qt 6.2+ com Widgets, Network, Svg e Test.

No Arch/CachyOS:

```bash
sudo pacman -Syu --needed base-devel cmake ninja qt6-base qt6-svg qt6-wayland
```

No Ubuntu/Debian:

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev libqt6svg6-dev qt6-wayland
```

Depois:

```bash
git clone https://github.com/uevertonpassos/Nox.git
cd Nox
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/nox
```

No Windows, use Qt 6 para MSVC 2022 e CMake. A compilação do instalador está em [.github/workflows/build.yml](.github/workflows/build.yml).

## Arquivos

- `src/main.cpp`: inicia o aplicativo.
- `src/janela.cpp`: janela, botões e busca.
- `src/sistema.cpp`: detecta o sistema e escolhe os comandos.
- `src/instalador.cpp`: baixa as informações de versão e executa a instalação.
- `resources/toolchain.sh`: instala Node.js e Go no Linux.
- `resources/style.qss`: aparência da janela.

## Testes

```bash
ctest --test-dir build --output-on-failure
```

Os testes verificam os comandos, a busca e uma instalação em pasta temporária. Não instalam ferramentas no computador durante o teste.

## Versões

O GitHub Actions compila Linux e Windows. Uma tag como `v0.1.0` publica os arquivos em Releases depois que a compilação e os testes terminam.

## Créditos

[Fontes dos logos](resources/logos/FONTES.md). As bibliotecas Qt e suas licenças estão descritas em [THIRD_PARTY.md](THIRD_PARTY.md).
