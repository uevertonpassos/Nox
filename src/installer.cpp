#include "language.h"
#include "installer.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVersionNumber>

Installer::Installer(QObject *parent) : QObject(parent) {
    process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&process, &QProcess::readyReadStandardOutput, this, [this]() {
        emit message(QString::fromLocal8Bit(process.readAllStandardOutput()));
    });
    connect(&process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            finish(false, trText("Could not start the command: ") + process.errorString());
    });
    connect(&process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus state) {
        emit message(QString::fromLocal8Bit(process.readAllStandardOutput()));
        bool success = code == 0 && state == QProcess::NormalExit;
        QString text = success ? trText("Installation completed. Open a new terminal to use the tool.") :
            trText("Installation failed (code ") + QString::number(code) + trText("). Check the log.");
        if (!success && currentPlan.administrator && (code == 126 || code == 127))
            text = trText("Authorization was cancelled or no authentication agent is available.");
        finish(success, text);
    });
}

bool Installer::busy() const {
    return working;
}

void Installer::finish(bool success, const QString &text) {
    if (!working)
        return;
    working = false;
    emit finished(success, text);
}

void Installer::install(const InstallPlan &plan, const SystemInfo &systemInfo) {
    if (working)
        return;
    working = true;
    currentPlan = plan;
    currentSystem = systemInfo;
    if (!plan.error.isEmpty()) {
        finish(false, plan.error);
        return;
    }
    if (systemInfo.family == "windows") {
        if (QStandardPaths::findExecutable("winget").isEmpty()) {
            finish(false, trText("WinGet was not found. Install or update App Installer from the Microsoft Store."));
            return;
        }
        run(plan.program, plan.arguments);
        return;
    }
    for (const QString &command : {QString("bash"), QString("curl")}) {
        if (QStandardPaths::findExecutable(command).isEmpty()) {
            finish(false, trText("Missing dependency: ") + command + trText(". Install it using your distribution package manager."));
            return;
        }
    }
    if (plan.tool == "node" || plan.tool == "npm" || plan.tool == "go") {
        for (const QString &command : {QString("tar"), QString("sha256sum"), QString("xz")}) {
            if (QStandardPaths::findExecutable(command).isEmpty()) {
                finish(false, trText("Missing dependency: ") + command);
                return;
            }
        }
        stage = plan.tool == "go" ? "go" : "node";
        fetch(stage == "go" ? "https://go.dev/dl/?mode=json" : "https://nodejs.org/dist/index.json");
        return;
    }
    QString script = "set -eu\n" + plan.commands;
    if (plan.administrator) {
        if (QStandardPaths::findExecutable("pkexec").isEmpty()) {
            finish(false, trText("Install polkit and an authentication agent for your desktop session."));
            return;
        }
        script = "set -eu\nexport PATH=/usr/sbin:/usr/bin:/sbin:/bin\nexport DEBIAN_FRONTEND=noninteractive\n" + plan.commands;
        run("pkexec", {"/bin/bash", "-c", script});
    } else {
        run("bash", {"-c", script});
    }
}

void Installer::fetch(const QString &url) {
    emit message(trText("Fetching ") + url + "\n");
    QNetworkRequest request{QUrl(url)};
    request.setTransferTimeout(30000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = network.get(request);
    connect(reply, &QNetworkReply::finished, this, &Installer::receiveReply);
}

void Installer::receiveReply() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QByteArray data = reply->readAll();
    bool failed = reply->error() != QNetworkReply::NoError;
    QString error = reply->errorString();
    reply->deleteLater();
    if (failed) {
        finish(false, trText("Could not fetch the official release: ") + error);
        return;
    }
    QString architecture = currentSystem.architecture == "x86_64" ? "amd64" : "arm64";
    if (stage == "checksum") {
        checksum.clear();
        for (const QByteArray &line : data.split('\n')) {
            QStringList parts = QString::fromUtf8(line).split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() == 2 && parts[1] == file)
                checksum = parts[0];
        }
        runToolchain();
        return;
    }
    QJsonArray releases = QJsonDocument::fromJson(data).array();
    QJsonObject selected;
    QVersionNumber highest;
    for (const QJsonValue &item : releases) {
        QJsonObject candidate = item.toObject();
        QString number = candidate.value("version").toString();
        bool stable = stage == "go" ? candidate.value("stable").toBool() : candidate.value("lts").isString();
        QString pattern = stage == "go" ? "^go[0-9]+\\.[0-9]+(?:\\.[0-9]+)?$" : "^v[0-9]+\\.[0-9]+\\.[0-9]+$";
        if (!stable || !QRegularExpression(pattern).match(number).hasMatch())
            continue;
        QVersionNumber current = QVersionNumber::fromString(number.mid(stage == "go" ? 2 : 1));
        if (current > highest) {
            highest = current;
            selected = candidate;
        }
    }
    version = selected.value("version").toString();
    if (version.isEmpty()) {
        finish(false, trText("The official website did not return a valid stable release."));
        return;
    }
    if (stage == "node") {
        QString cpu = architecture == "amd64" ? "x64" : "arm64";
        file = "node-" + version + "-linux-" + cpu + ".tar.xz";
        packageUrl = "https://nodejs.org/dist/" + version + "/" + file;
        stage = "checksum";
        fetch("https://nodejs.org/dist/" + version + "/SHASUMS256.txt");
        return;
    }
    for (const QJsonValue &item : selected.value("files").toArray()) {
        QJsonObject package = item.toObject();
        if (package.value("os") == "linux" && package.value("arch") == architecture && package.value("kind") == "archive") {
            file = package.value("filename").toString();
            if (!QRegularExpression("^go[0-9.]+\\.linux-(amd64|arm64)\\.tar\\.gz$").match(file).hasMatch())
                break;
            packageUrl = "https://go.dev/dl/" + file;
            checksum = package.value("sha256").toString();
            runToolchain();
            return;
        }
    }
    finish(false, trText("No Go package was found for this architecture."));
}

void Installer::runToolchain() {
    if (!QRegularExpression("^[a-fA-F0-9]{64}$").match(checksum).hasMatch()) {
        finish(false, trText("Official checksum is missing or invalid. No package was installed."));
        return;
    }
    QString tool = currentPlan.tool == "go" ? "go" : "node";
    emit message(trText("Selected version: ") + version + "\n");
    run("bash", {"-c", currentPlan.commands, "nox", tool, version, packageUrl, checksum});
}

void Installer::run(const QString &program, const QStringList &arguments) {
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert("NOX_LANGUAGE", currentLanguage());
    // Keep AppImage libraries out of host commands.
    for (const QString &name : {QString("LD_LIBRARY_PATH"), QString("LD_PRELOAD"), QString("QT_PLUGIN_PATH"), QString("QT_QPA_PLATFORM_PLUGIN_PATH")})
        environment.remove(name);
    process.setProcessEnvironment(environment);
    process.setStandardInputFile(QProcess::nullDevice());
    process.start(program, arguments);
}
