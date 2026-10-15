#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

struct Sistema {
    QString nome;
    QString id;
    QString familia;
    QString arquitetura;
    QString codinome;
    bool imutavel = false;
};

struct Plano {
    QString ferramenta;
    QString titulo;
    QString descricao;
    QString comandos;
    QString erro;
    QString programa;
    QStringList argumentos;
    bool administrador = false;
};

QMap<QString, QString> lerInformacoesLinux(const QString &texto);
Sistema identificarLinux(const QMap<QString, QString> &dados);
Sistema detectarSistema();
Plano criarPlano(const QString &ferramenta, const Sistema &sistema);
QString caminhoExecutavel(const QString &nome);
QString protegerArgumento(const QString &texto);
