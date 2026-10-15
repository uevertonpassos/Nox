#include "sistema.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSysInfo>

QMap<QString, QString> lerInformacoesLinux(const QString &texto) {
    QMap<QString, QString> dados;
    for (const QString &linha : texto.split('\n')) {
        int separador = linha.indexOf('=');
        if (separador < 1 || linha.startsWith('#'))
            continue;
        QString valor = linha.mid(separador + 1).trimmed();
        if ((valor.startsWith('"') && valor.endsWith('"')) ||
            (valor.startsWith('\'') && valor.endsWith('\'')))
            valor = valor.mid(1, valor.size() - 2);
        dados[linha.left(separador)] = valor;
    }
    return dados;
}

Sistema identificarLinux(const QMap<QString, QString> &dados) {
    Sistema sistema;
    sistema.id = dados.value("ID");
    sistema.nome = dados.value("PRETTY_NAME", sistema.id);
    sistema.codinome = dados.value("UBUNTU_CODENAME", dados.value("VERSION_CODENAME"));
    QStringList bases = dados.value("ID_LIKE").split(' ', Qt::SkipEmptyParts);
    bases.prepend(sistema.id);
    for (const QString &base : bases) {
        if (base == "ubuntu" || base == "debian")
            sistema.familia = "debian";
        else if (base == "arch" || base == "cachyos" || base == "manjaro")
            sistema.familia = "arch";
        else if (base == "fedora" || base == "rhel" || base == "centos")
            sistema.familia = "fedora";
        else if (base == "suse" || base.startsWith("opensuse"))
            sistema.familia = "suse";
        if (!sistema.familia.isEmpty())
            break;
    }
    return sistema;
}

Sistema detectarSistema() {
    Sistema sistema;
#ifdef Q_OS_WIN
    sistema.nome = QSysInfo::prettyProductName();
    sistema.id = "windows";
    sistema.familia = "windows";
#else
    QFile arquivo("/etc/os-release");
    if (arquivo.open(QIODevice::ReadOnly))
        sistema = identificarLinux(lerInformacoesLinux(QString::fromUtf8(arquivo.readAll())));
    sistema.imutavel = QFile::exists("/run/ostree-booted") || QFile::exists("/run/bootc");
#endif
    sistema.arquitetura = QSysInfo::currentCpuArchitecture();
    return sistema;
}

QString protegerArgumento(const QString &texto) {
    QString resultado = texto;
    resultado.replace("'", "'\\''");
    return "'" + resultado + "'";
}

QString caminhoExecutavel(const QString &nome) {
    QStringList pastas = {
        QDir::homePath() + "/.local/share/nox/bin",
        QDir::homePath() + "/.local/bin",
        "/usr/local/go/bin"
    };
#ifdef Q_OS_WIN
    pastas << qEnvironmentVariable("ProgramFiles") + "/nodejs"
           << qEnvironmentVariable("ProgramFiles") + "/Go/bin"
           << qEnvironmentVariable("ProgramFiles") + "/Docker/Docker/resources/bin"
           << qEnvironmentVariable("LOCALAPPDATA") + "/Microsoft/WinGet/Links"
           << qEnvironmentVariable("LOCALAPPDATA") + "/Programs/Zed";
#endif
    QString caminho = QStandardPaths::findExecutable(nome, pastas);
    if (caminho.isEmpty())
        caminho = QStandardPaths::findExecutable(nome);
    return caminho;
}

static QString instalarPacotes(const QString &familia, const QString &pacotes) {
    if (familia == "debian")
        return "apt-get update\napt-get install -y " + pacotes;
    if (familia == "arch")
        return "pacman -Syu --needed --noconfirm " + pacotes;
    if (familia == "fedora")
        return "dnf install -y " + pacotes;
    return "zypper --non-interactive install " + pacotes;
}

static QString repositorioDocker(const Sistema &sistema) {
    if (sistema.familia == "fedora") {
        QString base = sistema.id == "fedora" ? "fedora" : "rhel";
        return "curl -fL --proto '=https' --proto-redir '=https' https://download.docker.com/linux/" + base +
               "/docker-ce.repo -o /etc/yum.repos.d/docker-ce.repo\n";
    }
    QString repositorio = "deb [arch=amd64 signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/" +
                          sistema.id + " " + sistema.codinome + " stable\n";
    return "install -d -m 0755 /etc/apt/keyrings\n"
           "curl -fL --proto '=https' --proto-redir '=https' https://download.docker.com/linux/" + sistema.id +
           "/gpg -o /etc/apt/keyrings/docker.asc\nchmod a+r /etc/apt/keyrings/docker.asc\n"
           "printf %s " + protegerArgumento(repositorio) + " > /etc/apt/sources.list.d/nox-docker.list\napt-get update\n";
}

Plano criarPlano(const QString &ferramenta, const Sistema &sistema) {
    Plano plano;
    plano.ferramenta = ferramenta;
    QMap<QString, QString> nomes = {{"node", "Node.js LTS"}, {"npm", "npm"}, {"go", "Go estável"},
        {"zed", "Zed"}, {"docker-engine", "Docker Engine"}, {"docker-desktop", "Docker Desktop"},
        {"c", "Compilador C"}, {"cpp", "Compilador C++"}};
    plano.titulo = nomes.value(ferramenta);
    if (plano.titulo.isEmpty()) {
        plano.erro = "Ferramenta desconhecida.";
        return plano;
    }
    if (sistema.familia.isEmpty()) {
        plano.erro = "Esta distribuição ainda não tem um instalador disponível.";
        return plano;
    }
    if (sistema.arquitetura != "x86_64" && sistema.arquitetura != "arm64" && sistema.arquitetura != "aarch64") {
        plano.erro = "Esta versão atende a sistemas x86_64 e ARM64.";
        return plano;
    }
    if (sistema.familia == "windows") {
        if (ferramenta == "docker-engine") {
            plano.erro = "No Windows, use Docker Desktop. Docker Engine precisa de um ambiente Linux, como WSL.";
            return plano;
        }
        if ((ferramenta == "c" || ferramenta == "cpp") && sistema.arquitetura != "x86_64") {
            plano.erro = "O pacote de GCC para Windows usado pelo Nox requer x86_64.";
            return plano;
        }
        QMap<QString, QString> pacotes = {{"node", "OpenJS.NodeJS.LTS"}, {"npm", "OpenJS.NodeJS.LTS"},
            {"go", "GoLang.Go"}, {"zed", "ZedIndustries.Zed"}, {"docker-desktop", "Docker.DockerDesktop"},
            {"c", "BrechtSanders.WinLibs.POSIX.UCRT"}, {"cpp", "BrechtSanders.WinLibs.POSIX.UCRT"}};
        plano.programa = "winget";
        plano.argumentos = QStringList{"install", "--id", pacotes.value(ferramenta), "--exact", "--source", "winget",
            "--accept-package-agreements", "--accept-source-agreements", "--disable-interactivity"};
        plano.comandos = plano.programa + " " + plano.argumentos.join(' ');
        plano.descricao = "Instala pelo WinGet e aceita os termos do pacote e da fonte. O Windows pode pedir autorização. Abra um novo terminal após instalar.";
        if (ferramenta == "npm")
            plano.descricao += " npm será instalado junto com Node.js LTS.";
        if (ferramenta == "docker-desktop")
            plano.descricao += " Docker Desktop requer virtualização e WSL 2 ou Hyper-V; pode ser necessário reiniciar o Windows.";
        return plano;
    }
    if (ferramenta == "node" || ferramenta == "npm" || ferramenta == "go") {
        plano.descricao = "Baixa a versão oficial atual, verifica o SHA-256 e instala em ~/.local/share/nox. "
            "Adiciona o PATH a .profile, .bashrc, .zshrc e Fish. Abra um novo terminal depois de instalar. "
            "As versões do Nox passam a ter prioridade sobre as do sistema.";
        if (ferramenta == "npm")
            plano.descricao += " npm não tem LTS próprio: será instalado com o Node.js LTS compatível.";
        if (ferramenta == "go")
            plano.descricao += " Go não tem canal LTS; usamos a versão estável mais recente.";
        QFile receita(":/toolchain.sh");
        if (receita.open(QIODevice::ReadOnly))
            plano.comandos = QString::fromUtf8(receita.readAll());
        return plano;
    }
    if (ferramenta == "zed") {
        plano.descricao = "Executa o instalador oficial do Zed no seu usuário. Requer GPU com Vulkan e glibc recente. "
            "Se faltarem bibliotecas, o instalador pode solicitar dependências; instale-as pelo gerenciador da distribuição.";
        plano.comandos = "temporario=$(mktemp -d)\ntrap 'rm -rf -- \"$temporario\"' EXIT\n"
            "curl -fL --proto '=https' --proto-redir '=https' https://zed.dev/install.sh -o \"$temporario/zed.sh\"\n"
            "sh \"$temporario/zed.sh\"\n\"$HOME/.local/bin/zed\" --version";
        return plano;
    }
    plano.administrador = true;
    plano.descricao = "O sistema vai pedir sua senha para instalar os pacotes.";
    if (sistema.imutavel) {
        plano.erro = "A instalação de pacotes em sistemas imutáveis ainda não está disponível.";
        return plano;
    }
    if (ferramenta == "c" || ferramenta == "cpp") {
        QString pacotes = "gcc make";
        if (sistema.familia == "debian")
            pacotes = "build-essential";
        else if (sistema.familia == "arch")
            pacotes = "base-devel";
        else if (ferramenta == "cpp")
            pacotes = "gcc gcc-c++ make";
        plano.comandos = instalarPacotes(sistema.familia, pacotes) + "\n" +
                         (ferramenta == "c" ? "gcc --version" : "g++ --version");
    } else if (ferramenta == "docker-engine") {
        if (sistema.familia == "fedora")
            plano.comandos = repositorioDocker(sistema) + "dnf install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin";
        else if (sistema.familia == "arch")
            plano.comandos = instalarPacotes("arch", "docker docker-compose docker-buildx");
        else if (sistema.familia == "suse")
            plano.comandos = instalarPacotes("suse", "docker docker-compose");
        else
            plano.comandos = instalarPacotes("debian", "docker.io");
        plano.comandos += "\nsystemctl enable --now docker\ndocker info";
        plano.descricao += " Ativa o serviço agora e na inicialização. Use sudo docker no terminal. "
            "Debian/Ubuntu, Arch e openSUSE usam seus próprios repositórios; Fedora/RHEL usa o repositório Docker.";
    } else if (ferramenta == "docker-desktop") {
        if ((sistema.id != "ubuntu" && sistema.id != "debian" && sistema.id != "fedora" && sistema.id != "rhel") || sistema.arquitetura != "x86_64") {
            plano.erro = "Docker Desktop automático: Ubuntu, Debian, Fedora ou RHEL x86_64. "
                "No Arch/CachyOS o pacote é experimental; use Docker Engine.";
            return plano;
        }
        if (sistema.familia == "debian" && !QRegularExpression("^[a-z][a-z0-9-]+$").match(sistema.codinome).hasMatch()) {
            plano.erro = "Não foi possível identificar o codinome da distribuição.";
            return plano;
        }
        QString arquivo = sistema.familia == "debian" ? "docker-desktop-amd64.deb" : "docker-desktop-x86_64.rpm";
        plano.comandos = "test -e /dev/kvm || { echo 'Docker Desktop requer KVM habilitado.'; exit 1; }\n";
        plano.comandos += repositorioDocker(sistema);
        plano.comandos += "temporario=$(mktemp -d)\ntrap 'rm -rf -- \"$temporario\"' EXIT\nchmod 755 \"$temporario\"\n"
            "curl -fL --proto '=https' --proto-redir '=https' https://desktop.docker.com/linux/main/amd64/" + arquivo +
            " -o \"$temporario/" + arquivo + "\"\n";
        plano.comandos += (sistema.familia == "debian" ? "apt-get" : "dnf") + QString(" install -y \"$temporario/") + arquivo + "\"";
        plano.descricao += " Configura o repositório oficial Docker. Requer KVM, QEMU 5.2+, systemd e 4 GB de RAM. "
            "Abra o Desktop pelo menu para aceitar os termos. Alguns usos comerciais exigem assinatura.";
    }
    if (sistema.familia == "arch")
        plano.descricao += " No Arch/CachyOS, pacman -Syu também atualiza o sistema para evitar atualizações parciais.";
    return plano;
}
