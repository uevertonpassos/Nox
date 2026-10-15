#pragma once

#include "sistema.h"
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>

class Instalador : public QObject {
    Q_OBJECT
public:
    explicit Instalador(QObject *pai = nullptr);
    void instalar(const Plano &plano, const Sistema &sistema);
    bool ocupado() const;

signals:
    void mensagem(const QString &texto);
    void terminou(bool sucesso, const QString &texto);

private:
    void buscar(const QString &url);
    void receberResposta();
    void executar(const QString &programa, const QStringList &argumentos);
    void executarToolchain();
    void concluir(bool sucesso, const QString &texto);

    QProcess processo;
    QNetworkAccessManager rede;
    Plano planoAtual;
    Sistema sistemaAtual;
    QString etapa;
    QString versao;
    QString arquivo;
    QString urlPacote;
    QString checksum;
    bool trabalhando = false;
};
