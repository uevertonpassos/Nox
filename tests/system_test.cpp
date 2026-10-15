#include "../src/system.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QtTest>

class SystemTest : public QObject {
    Q_OBJECT
private slots:
    void distributions();
    void linuxPlans();
    void windowsPlans();
    void unsupportedSystems();
    void shellArguments();
    void isolatedInstallation();
};

void SystemTest::distributions() {
    const QList<QPair<QString, QString>> cases = {
        {"ID=ubuntu\nPRETTY_NAME=\"Ubuntu 24.04 LTS\"", "debian"},
        {"ID=debian", "debian"}, {"ID=linuxmint\nID_LIKE=\"ubuntu debian\"", "debian"},
        {"ID=cachyos\nID_LIKE=arch", "arch"}, {"ID=manjaro", "arch"},
        {"ID=fedora", "fedora"}, {"ID=rocky\nID_LIKE=\"rhel centos fedora\"", "fedora"},
        {"ID=opensuse-tumbleweed\nID_LIKE=\"opensuse suse\"", "suse"}, {"ID=nixos", ""}
    };
    for (const auto &testCase : cases)
        QCOMPARE(identifyLinux(parseOsRelease(testCase.first)).family, testCase.second);
    auto data = parseOsRelease("# comment\nPRETTY_NAME=\"Linux with spaces\"\nINVALID\nID=arch\n");
    QCOMPARE(data.value("PRETTY_NAME"), QString("Linux with spaces"));
}

void SystemTest::linuxPlans() {
    for (const QString &family : {QString("debian"), QString("arch"), QString("fedora"), QString("suse")}) {
        SystemInfo systemInfo;
        systemInfo.id = family;
        systemInfo.family = family;
        systemInfo.architecture = "x86_64";
        systemInfo.codename = "bookworm";
        for (const QString &id : {QString("node"), QString("npm"), QString("go"), QString("zed"), QString("c"), QString("cpp"), QString("docker-engine")}) {
            InstallPlan plan = createPlan(id, systemInfo);
            QVERIFY2(plan.error.isEmpty(), qPrintable(plan.error));
            QVERIFY(!plan.commands.isEmpty());
#ifndef Q_OS_WIN
            QProcess shell;
            shell.start("bash", {"-n", "-c", plan.commands});
            QVERIFY(shell.waitForFinished());
            QCOMPARE(shell.exitCode(), 0);
#endif
        }
        if (family == "arch") {
            QString commands = createPlan("cpp", systemInfo).commands;
            QVERIFY(commands.contains("-Syu"));
            QVERIFY(!commands.contains("-Sy "));
        }
    }
}

void SystemTest::windowsPlans() {
    SystemInfo systemInfo;
    systemInfo.family = "windows";
    systemInfo.architecture = "x86_64";
    QCOMPARE(createPlan("node", systemInfo).arguments, createPlan("npm", systemInfo).arguments);
    QVERIFY(createPlan("go", systemInfo).arguments.contains("GoLang.Go"));
    QVERIFY(createPlan("cpp", systemInfo).arguments.contains("BrechtSanders.WinLibs.POSIX.UCRT"));
    QVERIFY(!createPlan("docker-engine", systemInfo).error.isEmpty());
    QVERIFY(createPlan("docker-desktop", systemInfo).arguments.contains("Docker.DockerDesktop"));
}

void SystemTest::unsupportedSystems() {
    SystemInfo systemInfo;
    systemInfo.architecture = "x86_64";
    QVERIFY(!createPlan("c", systemInfo).error.isEmpty());
    systemInfo.family = "arch";
    systemInfo.id = "cachyos";
    QVERIFY(!createPlan("docker-desktop", systemInfo).error.isEmpty());
    systemInfo.immutable = true;
    QVERIFY(!createPlan("c", systemInfo).error.isEmpty());
    QVERIFY(createPlan("node", systemInfo).error.isEmpty());
    QVERIFY(!createPlan("unknown", systemInfo).error.isEmpty());
    systemInfo.architecture = "riscv64";
    QVERIFY(!createPlan("go", systemInfo).error.isEmpty());
}

void SystemTest::shellArguments() {
#ifndef Q_OS_WIN
    QString argument = "text ' with $(id) and `echo test`\nsecond line";
    QProcess shell;
    shell.start("bash", {"-c", "printf %s " + quoteArgument(argument)});
    QVERIFY(shell.waitForFinished());
    QCOMPARE(QString::fromUtf8(shell.readAllStandardOutput()), argument);
#endif
}

void SystemTest::isolatedInstallation() {
#ifdef Q_OS_WIN
    QSKIP("The toolchain recipe runs only on Linux.");
#else
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    QString root = temporary.path();
    QDir().mkpath(root + "/package/node/bin");
    QDir().mkpath(root + "/commands");
    QDir().mkpath(root + "/home");
    for (const QString &name : {QString("node"), QString("npm"), QString("npx")}) {
        QFile program(root + "/package/node/bin/" + name);
        QVERIFY(program.open(QIODevice::WriteOnly));
        program.write("#!/bin/sh\nprintf 'test version\\n'\n");
        program.close();
        QVERIFY(program.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    }
    QProcess tar;
    tar.start("tar", {"-czf", root + "/package.tar.gz", "-C", root + "/package", "node"});
    QVERIFY(tar.waitForFinished());
    QCOMPARE(tar.exitCode(), 0);
    QFile package(root + "/package.tar.gz");
    QVERIFY(package.open(QIODevice::ReadOnly));
    QString hash = QCryptographicHash::hash(package.readAll(), QCryptographicHash::Sha256).toHex();

    // Replace the download and run the recipe in a temporary HOME.
    QFile curl(root + "/commands/curl");
    QVERIFY(curl.open(QIODevice::WriteOnly));
    curl.write("#!/bin/bash\nwhile [ \"$1\" != '-o' ]; do shift; done\ncp \"$NOX_TEST_PACKAGE\" \"$2\"\n");
    curl.close();
    QVERIFY(curl.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    QFile recipe(":/toolchain.sh");
    QVERIFY(recipe.open(QIODevice::ReadOnly));
    QString script = QString::fromUtf8(recipe.readAll());
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert("HOME", root + "/home");
    environment.insert("PATH", root + "/commands:" + environment.value("PATH"));
    environment.insert("NOX_TEST_PACKAGE", root + "/package.tar.gz");
    QProcess process;
    process.setProcessEnvironment(environment);
    process.start("bash", {"-c", script, "nox", "node", "v24.0.0", "https://test.invalid/package", hash});
    QVERIFY(process.waitForFinished());
    QVERIFY2(process.exitCode() == 0, process.readAllStandardError().constData());
    QVERIFY(QFile::exists(root + "/home/.local/share/nox/bin/node"));
    QFile profile(root + "/home/.bashrc");
    QVERIFY(profile.open(QIODevice::ReadOnly));
    QByteArray previousProfile = profile.readAll();
    QVERIFY(previousProfile.contains(".config/nox/env.sh"));
    profile.close();

    // A corrupt download must leave the active installation intact.
    QString previousLink = QFileInfo(root + "/home/.local/share/nox/current/node").symLinkTarget();
    process.start("bash", {"-c", script, "nox", "node", "v24.0.0", "https://test.invalid/package", QString(64, '0')});
    QVERIFY(process.waitForFinished());
    QVERIFY(process.exitCode() != 0);
    QCOMPARE(QFileInfo(root + "/home/.local/share/nox/current/node").symLinkTarget(), previousLink);

    // Reinstallation must not duplicate the shell configuration.
    process.start("bash", {"-c", script, "nox", "node", "v24.0.0", "https://test.invalid/package", hash});
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitCode(), 0);
    QVERIFY(profile.open(QIODevice::ReadOnly));
    QCOMPARE(profile.readAll(), previousProfile);
#endif
}

QTEST_GUILESS_MAIN(SystemTest)
#include "system_test.moc"
