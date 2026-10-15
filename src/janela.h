#pragma once

#include "instalador.h"
#include "sistema.h"
#include <QMainWindow>
#include <QMap>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;

class Janela : public QMainWindow {
    Q_OBJECT
public:
    explicit Janela(QWidget *pai = nullptr);

protected:
    void closeEvent(QCloseEvent *evento) override;

private:
    QWidget *criarLinha(const QString &id, const QString &nome, const QString &descricao);
    void escolherDocker();
    void confirmarInstalacao(const QString &id);
    void atualizarDeteccao();
    void filtrarFerramentas(const QString &busca);
    void escreverRegistro(const QString &texto);
    void definirOcupado(bool ocupado);
    void salvarRegistro();

    Sistema sistema;
    Instalador instalador;
    QMap<QString, QWidget *> ferramentas;
    QMap<QString, QLabel *> situacoes;
    QList<QPushButton *> botoesInstalar;
    QLineEdit *campoBusca;
    QPlainTextEdit *registro;
    QLabel *mensagem;
    QLabel *semResultados;
    QPushButton *botaoAtualizar;
    QProgressBar *progresso;
};
