#include "janela.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QLockFile>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTimer>
#ifndef Q_OS_WIN
#include <unistd.h>
#endif

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("nox");
    app.setOrganizationName("Nox");
    app.setApplicationVersion("0.1.0");
    app.setStyle("Fusion");

    QFile estilo(":/style.qss");
    if (estilo.open(QIODevice::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(estilo.readAll()));

    bool teste = app.arguments().contains("--smoke-test");
#ifndef Q_OS_WIN
    if (geteuid() == 0 && !teste) {
        QMessageBox::warning(nullptr, "Nox", "Abra o Nox como usuário normal. O sistema pedirá a senha quando necessário.");
        return 1;
    }
#endif
    QString pasta = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(pasta);
    QLockFile trava(pasta + "/nox.lock");
    if (!teste && !trava.tryLock(0)) {
        QMessageBox::information(nullptr, "Nox", "O Nox já está aberto em outra janela.");
        return 1;
    }

    Janela janela;
    janela.show();

    if (teste) {
        // Exercita a janela e a busca sem executar instalações.
        QTimer::singleShot(0, &app, [&app, &janela]() {
            QLineEdit *busca = janela.findChild<QLineEdit *>("busca");
            QPushButton *node = janela.findChild<QPushButton *>("instalar_node");
            QPushButton *go = janela.findChild<QPushButton *>("instalar_go");
            if (!busca || !node || !go) {
                app.exit(1);
                return;
            }
            busca->setText("Go");
            bool correto = go->isVisible() && !node->isVisible();
            busca->clear();
            correto = correto && node->isVisible();
            app.exit(correto ? 0 : 1);
        });
    }
    int captura = app.arguments().indexOf("--screenshot");
    if (captura > 0 && captura + 1 < app.arguments().size()) {
        QString caminho = app.arguments()[captura + 1];
        QTimer::singleShot(500, &app, [&app, &janela, caminho]() {
            app.exit(janela.grab().save(caminho) ? 0 : 1);
        });
    }
    return app.exec();
}
