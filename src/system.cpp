#include "language.h"
#include "system.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSysInfo>

QMap<QString, QString> parseOsRelease(const QString &text) {
    QMap<QString, QString> data;
    for (const QString &line : text.split('\n')) {
        int separator = line.indexOf('=');
        if (separator < 1 || line.startsWith('#'))
            continue;
        QString value = line.mid(separator + 1).trimmed();
        if ((value.startsWith('"') && value.endsWith('"')) ||
            (value.startsWith('\'') && value.endsWith('\'')))
            value = value.mid(1, value.size() - 2);
        data[line.left(separator)] = value;
    }
    return data;
}

SystemInfo identifyLinux(const QMap<QString, QString> &data) {
    SystemInfo systemInfo;
    systemInfo.id = data.value("ID");
    systemInfo.name = data.value("PRETTY_NAME", systemInfo.id);
    systemInfo.codename = data.value("UBUNTU_CODENAME", data.value("VERSION_CODENAME"));
    QStringList bases = data.value("ID_LIKE").split(' ', Qt::SkipEmptyParts);
    bases.prepend(systemInfo.id);
    for (const QString &base : bases) {
        if (base == "ubuntu" || base == "debian")
            systemInfo.family = "debian";
        else if (base == "arch" || base == "cachyos" || base == "manjaro")
            systemInfo.family = "arch";
        else if (base == "fedora" || base == "rhel" || base == "centos")
            systemInfo.family = "fedora";
        else if (base == "suse" || base.startsWith("opensuse"))
            systemInfo.family = "suse";
        if (!systemInfo.family.isEmpty())
            break;
    }
    return systemInfo;
}

SystemInfo detectSystem() {
    SystemInfo systemInfo;
#ifdef Q_OS_WIN
    systemInfo.name = QSysInfo::prettyProductName();
    systemInfo.id = "windows";
    systemInfo.family = "windows";
#else
    QFile file("/etc/os-release");
    if (file.open(QIODevice::ReadOnly))
        systemInfo = identifyLinux(parseOsRelease(QString::fromUtf8(file.readAll())));
    systemInfo.immutable = QFile::exists("/run/ostree-booted") || QFile::exists("/run/bootc");
#endif
    systemInfo.architecture = QSysInfo::currentCpuArchitecture();
    return systemInfo;
}

QString quoteArgument(const QString &text) {
    QString result = text;
    result.replace("'", "'\\''");
    return "'" + result + "'";
}

QString findExecutable(const QString &name) {
    QStringList folders = {
        QDir::homePath() + "/.local/share/nox/bin",
        QDir::homePath() + "/.local/bin",
        "/usr/local/go/bin"
    };
#ifdef Q_OS_WIN
    folders << qEnvironmentVariable("ProgramFiles") + "/nodejs"
           << qEnvironmentVariable("ProgramFiles") + "/Go/bin"
           << qEnvironmentVariable("ProgramFiles") + "/Docker/Docker/resources/bin"
           << qEnvironmentVariable("LOCALAPPDATA") + "/Microsoft/WinGet/Links"
           << qEnvironmentVariable("LOCALAPPDATA") + "/Programs/Zed";
#endif
    QString path = QStandardPaths::findExecutable(name, folders);
    if (path.isEmpty())
        path = QStandardPaths::findExecutable(name);
    return path;
}

static QString installPackages(const QString &family, const QString &packages) {
    if (family == "debian")
        return "apt-get update\napt-get install -y " + packages;
    if (family == "arch")
        return "pacman -Syu --needed --noconfirm " + packages;
    if (family == "fedora")
        return "dnf install -y " + packages;
    return "zypper --non-interactive install " + packages;
}

static QString dockerRepository(const SystemInfo &systemInfo) {
    if (systemInfo.family == "fedora") {
        QString base = systemInfo.id == "fedora" ? "fedora" : "rhel";
        return "curl -fL --proto '=https' --proto-redir '=https' https://download.docker.com/linux/" + base +
               "/docker-ce.repo -o /etc/yum.repos.d/docker-ce.repo\n";
    }
    QString repository = "deb [arch=amd64 signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/" +
                          systemInfo.id + " " + systemInfo.codename + " stable\n";
    return "install -d -m 0755 /etc/apt/keyrings\n"
           "curl -fL --proto '=https' --proto-redir '=https' https://download.docker.com/linux/" + systemInfo.id +
           "/gpg -o /etc/apt/keyrings/docker.asc\nchmod a+r /etc/apt/keyrings/docker.asc\n"
           "printf %s " + quoteArgument(repository) + " > /etc/apt/sources.list.d/nox-docker.list\napt-get update\n";
}

InstallPlan createPlan(const QString &tool, const SystemInfo &systemInfo) {
    InstallPlan plan;
    plan.tool = tool;
    QMap<QString, QString> names = {{"node", "Node.js LTS"}, {"npm", "npm"}, {"go", trText("Go (stable)")},
        {"zed", "Zed"}, {"docker-engine", "Docker Engine"}, {"docker-desktop", "Docker Desktop"},
        {"c", trText("C compiler")}, {"cpp", trText("C++ compiler")}};
    plan.title = names.value(tool);
    if (plan.title.isEmpty()) {
        plan.error = trText("Unknown tool.");
        return plan;
    }
    if (systemInfo.family.isEmpty()) {
        plan.error = trText("This distribution is not supported yet.");
        return plan;
    }
    if (systemInfo.architecture != "x86_64" && systemInfo.architecture != "arm64" && systemInfo.architecture != "aarch64") {
        plan.error = trText("This release supports x86_64 and ARM64 systems.");
        return plan;
    }
    if (systemInfo.family == "windows") {
        if (tool == "docker-engine") {
            plan.error = trText("Use Docker Desktop on Windows. Docker Engine requires a Linux environment such as WSL.");
            return plan;
        }
        if ((tool == "c" || tool == "cpp") && systemInfo.architecture != "x86_64") {
            plan.error = trText("The GCC package used by Nox on Windows requires x86_64.");
            return plan;
        }
        QMap<QString, QString> packages = {{"node", "OpenJS.NodeJS.LTS"}, {"npm", "OpenJS.NodeJS.LTS"},
            {"go", "GoLang.Go"}, {"zed", "ZedIndustries.Zed"}, {"docker-desktop", "Docker.DockerDesktop"},
            {"c", "BrechtSanders.WinLibs.POSIX.UCRT"}, {"cpp", "BrechtSanders.WinLibs.POSIX.UCRT"}};
        plan.program = "winget";
        plan.arguments = QStringList{"install", "--id", packages.value(tool), "--exact", "--source", "winget",
            "--accept-package-agreements", "--accept-source-agreements", "--disable-interactivity"};
        plan.commands = plan.program + " " + plan.arguments.join(' ');
        plan.description = trText("Installs through WinGet and accepts package and source agreements. Windows may ask for authorization. Open a new terminal after installation.");
        if (tool == "npm")
            plan.description += trText(" npm will be installed with Node.js LTS.");
        if (tool == "docker-desktop")
            plan.description += trText(" Docker Desktop requires virtualization and WSL 2 or Hyper-V. You may need to restart Windows.");
        return plan;
    }
    if (tool == "node" || tool == "npm" || tool == "go") {
        plan.description = trText("Downloads the latest official release, verifies its SHA-256, and installs it in ~/.local/share/nox. Adds the PATH to .profile, .bashrc, .zshrc, and Fish. Open a new terminal afterwards. Nox versions take priority over system versions.");
        if (tool == "npm")
            plan.description += trText(" npm has no LTS channel of its own. It will be installed with a compatible Node.js LTS release.");
        if (tool == "go")
            plan.description += trText(" Go has no LTS channel. Nox uses the latest stable release.");
        QFile recipe(":/toolchain.sh");
        if (recipe.open(QIODevice::ReadOnly))
            plan.commands = QString::fromUtf8(recipe.readAll());
        return plan;
    }
    if (tool == "zed") {
        plan.description = trText("Runs the official Zed installer for your user. Requires a Vulkan-capable GPU and a recent glibc. If libraries are missing, install the required dependencies through your distribution package manager.");
        plan.commands = "temporary=$(mktemp -d)\ntrap 'rm -rf -- \"$temporary\"' EXIT\ncurl -fL --proto '=https' --proto-redir '=https' https://zed.dev/install.sh -o \"$temporary/zed.sh\"\nsh \"$temporary/zed.sh\"\n\"$HOME/.local/bin/zed\" --version";
        return plan;
    }
    plan.administrator = true;
    plan.description = trText("The system will ask for your password to install the packages.");
    if (systemInfo.immutable) {
        plan.error = trText("Package installation on immutable systems is not supported yet.");
        return plan;
    }
    if (tool == "c" || tool == "cpp") {
        QString packages = "gcc make";
        if (systemInfo.family == "debian")
            packages = "build-essential";
        else if (systemInfo.family == "arch")
            packages = "base-devel";
        else if (tool == "cpp")
            packages = "gcc gcc-c++ make";
        plan.commands = installPackages(systemInfo.family, packages) + "\n" +
                         (tool == "c" ? "gcc --version" : "g++ --version");
    } else if (tool == "docker-engine") {
        if (systemInfo.family == "fedora")
            plan.commands = dockerRepository(systemInfo) + "dnf install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin";
        else if (systemInfo.family == "arch")
            plan.commands = installPackages("arch", "docker docker-compose docker-buildx");
        else if (systemInfo.family == "suse")
            plan.commands = installPackages("suse", "docker docker-compose");
        else
            plan.commands = installPackages("debian", "docker.io");
        plan.commands += "\nsystemctl enable --now docker\ndocker info";
        plan.description += trText(" Starts the service now and at boot. Use sudo docker in the terminal. Debian/Ubuntu, Arch, and openSUSE use their own repositories; Fedora/RHEL uses the Docker repository.");
    } else if (tool == "docker-desktop") {
        if ((systemInfo.id != "ubuntu" && systemInfo.id != "debian" && systemInfo.id != "fedora" && systemInfo.id != "rhel") || systemInfo.architecture != "x86_64") {
            plan.error = trText("Automatic Docker Desktop installation supports Ubuntu, Debian, Fedora, and RHEL x86_64. The Arch/CachyOS package is experimental; use Docker Engine.");
            return plan;
        }
        if (systemInfo.family == "debian" && !QRegularExpression("^[a-z][a-z0-9-]+$").match(systemInfo.codename).hasMatch()) {
            plan.error = trText("Could not identify the distribution codename.");
            return plan;
        }
        QString file = systemInfo.family == "debian" ? "docker-desktop-amd64.deb" : "docker-desktop-x86_64.rpm";
        plan.commands = "test -e /dev/kvm || { echo " + quoteArgument(trText("Docker Desktop requires KVM.")) + "; exit 1; }\n";
        plan.commands += dockerRepository(systemInfo);
        plan.commands += "temporary=$(mktemp -d)\ntrap 'rm -rf -- \"$temporary\"' EXIT\nchmod 755 \"$temporary\"\ncurl -fL --proto '=https' --proto-redir '=https' https://desktop.docker.com/linux/main/amd64/" + file +
            " -o \"$temporary/" + file + "\"\n";
        plan.commands += (systemInfo.family == "debian" ? "apt-get" : "dnf") + QString(" install -y \"$temporary/") + file + "\"";
        plan.description += trText(" Adds the official Docker repository. Requires KVM, QEMU 5.2+, systemd, and 4 GB RAM. Open Desktop from the application menu to accept its terms. Some commercial uses require a subscription.");
    }
    if (systemInfo.family == "arch")
        plan.description += trText(" On Arch/CachyOS, pacman -Syu also updates the system to avoid partial upgrades.");
    return plan;
}
