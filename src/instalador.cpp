#include "instalador.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVersionNumber>

Instalador::Instalador(QObject *pai) : QObject(pai) {
    processo.setProcessChannelMode(QProcess::MergedChannels);
    connect(&processo, &QProcess::readyReadStandardOutput, this, [this]() {
        emit mensagem(QString::fromLocal8Bit(processo.readAllStandardOutput()));
    });
    connect(&processo, &QProcess::errorOccurred, this, [this](QProcess::ProcessError erro) {
        if (erro == QProcess::FailedToStart)
            concluir(false, "Não foi possível iniciar o comando: " + processo.errorString());
    });
    connect(&processo, &QProcess::finished, this, [this](int codigo, QProcess::ExitStatus estado) {
        emit mensagem(QString::fromLocal8Bit(processo.readAllStandardOutput()));
        bool sucesso = codigo == 0 && estado == QProcess::NormalExit;
        QString texto = sucesso ? "Instalação concluída. Abra um novo terminal para usar a ferramenta." :
            "A instalação não foi concluída (código " + QString::number(codigo) + "). Confira o registro.";
        if (!sucesso && planoAtual.administrador && (codigo == 126 || codigo == 127))
            texto = "Autorização cancelada ou agente de autenticação indisponível.";
        concluir(sucesso, texto);
    });
}

bool Instalador::ocupado() const {
    return trabalhando;
}

void Instalador::concluir(bool sucesso, const QString &texto) {
    if (!trabalhando)
        return;
    trabalhando = false;
    emit terminou(sucesso, texto);
}

void Instalador::instalar(const Plano &plano, const Sistema &sistema) {
    if (trabalhando)
        return;
    trabalhando = true;
    planoAtual = plano;
    sistemaAtual = sistema;
    if (!plano.erro.isEmpty()) {
        concluir(false, plano.erro);
        return;
    }
    if (sistema.familia == "windows") {
        if (QStandardPaths::findExecutable("winget").isEmpty()) {
            concluir(false, "WinGet não encontrado. Instale ou atualize o Instalador de Aplicativo pela Microsoft Store.");
            return;
        }
        executar(plano.programa, plano.argumentos);
        return;
    }
    for (const QString &comando : {QString("bash"), QString("curl")}) {
        if (QStandardPaths::findExecutable(comando).isEmpty()) {
            concluir(false, "Dependência ausente: " + comando + ". Instale pelo gerenciador da distribuição.");
            return;
        }
    }
    if (plano.ferramenta == "node" || plano.ferramenta == "npm" || plano.ferramenta == "go") {
        for (const QString &comando : {QString("tar"), QString("sha256sum"), QString("xz")}) {
            if (QStandardPaths::findExecutable(comando).isEmpty()) {
                concluir(false, "Dependência ausente: " + comando);
                return;
            }
        }
        etapa = plano.ferramenta == "go" ? "go" : "node";
        buscar(etapa == "go" ? "https://go.dev/dl/?mode=json" : "https://nodejs.org/dist/index.json");
        return;
    }
    QString script = "set -eu\n" + plano.comandos;
    if (plano.administrador) {
        if (QStandardPaths::findExecutable("pkexec").isEmpty()) {
            concluir(false, "Instale polkit e um agente de autenticação para sua sessão desktop.");
            return;
        }
        script = "set -eu\nexport PATH=/usr/sbin:/usr/bin:/sbin:/bin\nexport DEBIAN_FRONTEND=noninteractive\n" + plano.comandos;
        executar("pkexec", {"/bin/bash", "-c", script});
    } else {
        executar("bash", {"-c", script});
    }
}

void Instalador::buscar(const QString &url) {
    emit mensagem("Consultando " + url + "\n");
    QNetworkRequest pedido{QUrl(url)};
    pedido.setTransferTimeout(30000);
    pedido.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *resposta = rede.get(pedido);
    connect(resposta, &QNetworkReply::finished, this, &Instalador::receberResposta);
}

void Instalador::receberResposta() {
    QNetworkReply *resposta = qobject_cast<QNetworkReply *>(sender());
    QByteArray dados = resposta->readAll();
    bool falhou = resposta->error() != QNetworkReply::NoError;
    QString erro = resposta->errorString();
    resposta->deleteLater();
    if (falhou) {
        concluir(false, "Falha ao consultar a versão oficial: " + erro);
        return;
    }
    QString arquitetura = sistemaAtual.arquitetura == "x86_64" ? "amd64" : "arm64";
    if (etapa == "checksum") {
        checksum.clear();
        for (const QByteArray &linha : dados.split('\n')) {
            QStringList partes = QString::fromUtf8(linha).split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (partes.size() == 2 && partes[1] == arquivo)
                checksum = partes[0];
        }
        executarToolchain();
        return;
    }
    QJsonArray versoes = QJsonDocument::fromJson(dados).array();
    QJsonObject selecionada;
    QVersionNumber maior;
    for (const QJsonValue &item : versoes) {
        QJsonObject candidato = item.toObject();
        QString numero = candidato.value("version").toString();
        bool estavel = etapa == "go" ? candidato.value("stable").toBool() : candidato.value("lts").isString();
        QString padrao = etapa == "go" ? "^go[0-9]+\\.[0-9]+(?:\\.[0-9]+)?$" : "^v[0-9]+\\.[0-9]+\\.[0-9]+$";
        if (!estavel || !QRegularExpression(padrao).match(numero).hasMatch())
            continue;
        QVersionNumber atual = QVersionNumber::fromString(numero.mid(etapa == "go" ? 2 : 1));
        if (atual > maior) {
            maior = atual;
            selecionada = candidato;
        }
    }
    versao = selecionada.value("version").toString();
    if (versao.isEmpty()) {
        concluir(false, "O site oficial não retornou uma versão estável válida.");
        return;
    }
    if (etapa == "node") {
        QString cpu = arquitetura == "amd64" ? "x64" : "arm64";
        arquivo = "node-" + versao + "-linux-" + cpu + ".tar.xz";
        urlPacote = "https://nodejs.org/dist/" + versao + "/" + arquivo;
        etapa = "checksum";
        buscar("https://nodejs.org/dist/" + versao + "/SHASUMS256.txt");
        return;
    }
    for (const QJsonValue &item : selecionada.value("files").toArray()) {
        QJsonObject pacote = item.toObject();
        if (pacote.value("os") == "linux" && pacote.value("arch") == arquitetura && pacote.value("kind") == "archive") {
            arquivo = pacote.value("filename").toString();
            if (!QRegularExpression("^go[0-9.]+\\.linux-(amd64|arm64)\\.tar\\.gz$").match(arquivo).hasMatch())
                break;
            urlPacote = "https://go.dev/dl/" + arquivo;
            checksum = pacote.value("sha256").toString();
            executarToolchain();
            return;
        }
    }
    concluir(false, "Não foi encontrado um pacote Go para esta arquitetura.");
}

void Instalador::executarToolchain() {
    if (!QRegularExpression("^[a-fA-F0-9]{64}$").match(checksum).hasMatch()) {
        concluir(false, "Checksum oficial ausente ou inválido. Nenhum pacote foi instalado.");
        return;
    }
    QString ferramenta = planoAtual.ferramenta == "go" ? "go" : "node";
    emit mensagem("Versão selecionada: " + versao + "\n");
    executar("bash", {"-c", planoAtual.comandos, "nox", ferramenta, versao, urlPacote, checksum});
}

void Instalador::executar(const QString &programa, const QStringList &argumentos) {
    QProcessEnvironment ambiente = QProcessEnvironment::systemEnvironment();
    // Evita que bibliotecas de um AppImage sejam usadas pelos comandos do sistema.
    for (const QString &nome : {QString("LD_LIBRARY_PATH"), QString("LD_PRELOAD"), QString("QT_PLUGIN_PATH"), QString("QT_QPA_PLATFORM_PLUGIN_PATH")})
        ambiente.remove(nome);
    processo.setProcessEnvironment(ambiente);
    processo.setStandardInputFile(QProcess::nullDevice());
    processo.start(programa, argumentos);
}
