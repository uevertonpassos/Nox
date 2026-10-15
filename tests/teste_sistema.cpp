#include "../src/sistema.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QtTest>

class TesteSistema : public QObject {
    Q_OBJECT
private slots:
    void distribuicoes();
    void receitasLinux();
    void receitasWindows();
    void sistemasSemSuporte();
    void argumentosDoShell();
    void instalacaoIsolada();
};

void TesteSistema::distribuicoes() {
    const QList<QPair<QString, QString>> casos = {
        {"ID=ubuntu\nPRETTY_NAME=\"Ubuntu 24.04 LTS\"", "debian"},
        {"ID=debian", "debian"}, {"ID=linuxmint\nID_LIKE=\"ubuntu debian\"", "debian"},
        {"ID=cachyos\nID_LIKE=arch", "arch"}, {"ID=manjaro", "arch"},
        {"ID=fedora", "fedora"}, {"ID=rocky\nID_LIKE=\"rhel centos fedora\"", "fedora"},
        {"ID=opensuse-tumbleweed\nID_LIKE=\"opensuse suse\"", "suse"}, {"ID=nixos", ""}
    };
    for (const auto &caso : casos)
        QCOMPARE(identificarLinux(lerInformacoesLinux(caso.first)).familia, caso.second);
    auto dados = lerInformacoesLinux("# comentário\nPRETTY_NAME=\"Linux com espaços\"\nINVALIDO\nID=arch\n");
    QCOMPARE(dados.value("PRETTY_NAME"), QString("Linux com espaços"));
}

void TesteSistema::receitasLinux() {
    for (const QString &familia : {QString("debian"), QString("arch"), QString("fedora"), QString("suse")}) {
        Sistema sistema;
        sistema.id = familia;
        sistema.familia = familia;
        sistema.arquitetura = "x86_64";
        sistema.codinome = "bookworm";
        for (const QString &id : {QString("node"), QString("npm"), QString("go"), QString("zed"), QString("c"), QString("cpp"), QString("docker-engine")}) {
            Plano plano = criarPlano(id, sistema);
            QVERIFY2(plano.erro.isEmpty(), qPrintable(plano.erro));
            QVERIFY(!plano.comandos.isEmpty());
#ifndef Q_OS_WIN
            QProcess shell;
            shell.start("bash", {"-n", "-c", plano.comandos});
            QVERIFY(shell.waitForFinished());
            QCOMPARE(shell.exitCode(), 0);
#endif
        }
        if (familia == "arch") {
            QString comandos = criarPlano("cpp", sistema).comandos;
            QVERIFY(comandos.contains("-Syu"));
            QVERIFY(!comandos.contains("-Sy "));
        }
    }
}

void TesteSistema::receitasWindows() {
    Sistema sistema;
    sistema.familia = "windows";
    sistema.arquitetura = "x86_64";
    QCOMPARE(criarPlano("node", sistema).argumentos, criarPlano("npm", sistema).argumentos);
    QVERIFY(criarPlano("go", sistema).argumentos.contains("GoLang.Go"));
    QVERIFY(criarPlano("cpp", sistema).argumentos.contains("BrechtSanders.WinLibs.POSIX.UCRT"));
    QVERIFY(!criarPlano("docker-engine", sistema).erro.isEmpty());
    QVERIFY(criarPlano("docker-desktop", sistema).argumentos.contains("Docker.DockerDesktop"));
}

void TesteSistema::sistemasSemSuporte() {
    Sistema sistema;
    sistema.arquitetura = "x86_64";
    QVERIFY(!criarPlano("c", sistema).erro.isEmpty());
    sistema.familia = "arch";
    sistema.id = "cachyos";
    QVERIFY(!criarPlano("docker-desktop", sistema).erro.isEmpty());
    sistema.imutavel = true;
    QVERIFY(!criarPlano("c", sistema).erro.isEmpty());
    QVERIFY(criarPlano("node", sistema).erro.isEmpty());
    QVERIFY(!criarPlano("desconhecido", sistema).erro.isEmpty());
    sistema.arquitetura = "riscv64";
    QVERIFY(!criarPlano("go", sistema).erro.isEmpty());
}

void TesteSistema::argumentosDoShell() {
#ifndef Q_OS_WIN
    QString argumento = "texto ' com $(id) e `echo teste`\nsegunda linha";
    QProcess shell;
    shell.start("bash", {"-c", "printf %s " + protegerArgumento(argumento)});
    QVERIFY(shell.waitForFinished());
    QCOMPARE(QString::fromUtf8(shell.readAllStandardOutput()), argumento);
#endif
}

void TesteSistema::instalacaoIsolada() {
#ifdef Q_OS_WIN
    QSKIP("A receita de toolchains é executada somente no Linux.");
#else
    QTemporaryDir temporario;
    QVERIFY(temporario.isValid());
    QString raiz = temporario.path();
    QDir().mkpath(raiz + "/pacote/node/bin");
    QDir().mkpath(raiz + "/comandos");
    QDir().mkpath(raiz + "/home");
    for (const QString &nome : {QString("node"), QString("npm"), QString("npx")}) {
        QFile programa(raiz + "/pacote/node/bin/" + nome);
        QVERIFY(programa.open(QIODevice::WriteOnly));
        programa.write("#!/bin/sh\nprintf 'versao de teste\\n'\n");
        programa.close();
        QVERIFY(programa.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    }
    QProcess tar;
    tar.start("tar", {"-czf", raiz + "/pacote.tar.gz", "-C", raiz + "/pacote", "node"});
    QVERIFY(tar.waitForFinished());
    QCOMPARE(tar.exitCode(), 0);
    QFile pacote(raiz + "/pacote.tar.gz");
    QVERIFY(pacote.open(QIODevice::ReadOnly));
    QString hash = QCryptographicHash::hash(pacote.readAll(), QCryptographicHash::Sha256).toHex();

    // Substitui apenas o download. O restante da receita roda numa HOME temporária.
    QFile curl(raiz + "/comandos/curl");
    QVERIFY(curl.open(QIODevice::WriteOnly));
    curl.write("#!/bin/bash\nwhile [ \"$1\" != '-o' ]; do shift; done\ncp \"$NOX_PACOTE_TESTE\" \"$2\"\n");
    curl.close();
    QVERIFY(curl.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    QFile receita(":/toolchain.sh");
    QVERIFY(receita.open(QIODevice::ReadOnly));
    QString script = QString::fromUtf8(receita.readAll());
    QProcessEnvironment ambiente = QProcessEnvironment::systemEnvironment();
    ambiente.insert("HOME", raiz + "/home");
    ambiente.insert("PATH", raiz + "/comandos:" + ambiente.value("PATH"));
    ambiente.insert("NOX_PACOTE_TESTE", raiz + "/pacote.tar.gz");
    QProcess processo;
    processo.setProcessEnvironment(ambiente);
    processo.start("bash", {"-c", script, "nox", "node", "v24.0.0", "https://teste.invalid/pacote", hash});
    QVERIFY(processo.waitForFinished());
    QVERIFY2(processo.exitCode() == 0, processo.readAllStandardError().constData());
    QVERIFY(QFile::exists(raiz + "/home/.local/share/nox/bin/node"));
    QFile perfil(raiz + "/home/.bashrc");
    QVERIFY(perfil.open(QIODevice::ReadOnly));
    QByteArray perfilAntes = perfil.readAll();
    QVERIFY(perfilAntes.contains(".config/nox/env.sh"));
    perfil.close();

    // Um download corrompido deve falhar sem trocar a instalação existente.
    QString linkAntes = QFileInfo(raiz + "/home/.local/share/nox/current/node").symLinkTarget();
    processo.start("bash", {"-c", script, "nox", "node", "v24.0.0", "https://teste.invalid/pacote", QString(64, '0')});
    QVERIFY(processo.waitForFinished());
    QVERIFY(processo.exitCode() != 0);
    QCOMPARE(QFileInfo(raiz + "/home/.local/share/nox/current/node").symLinkTarget(), linkAntes);

    // Uma reinstalação não deve duplicar a configuração do terminal.
    processo.start("bash", {"-c", script, "nox", "node", "v24.0.0", "https://teste.invalid/pacote", hash});
    QVERIFY(processo.waitForFinished());
    QCOMPARE(processo.exitCode(), 0);
    QVERIFY(perfil.open(QIODevice::ReadOnly));
    QCOMPARE(perfil.readAll(), perfilAntes);
#endif
}

QTEST_GUILESS_MAIN(TesteSistema)
#include "teste_sistema.moc"
