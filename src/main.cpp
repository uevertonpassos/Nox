#include "language.h"
#include "theme.h"
#include "window.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QFont>
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
    app.setApplicationVersion("0.2.0");
    app.setDesktopFileName("io.noxdev.Nox");
    app.setWindowIcon(QIcon(":/nox.svg"));
    loadLanguage();
    app.setStyle("Fusion");
#ifdef Q_OS_WIN
    app.setFont(QFont("Segoe UI", 10));
#endif

    loadTheme();

    bool smokeTest = app.arguments().contains("--smoke-test");
#ifndef Q_OS_WIN
    if (geteuid() == 0 && !smokeTest) {
        QMessageBox::warning(nullptr, "Nox", trText("Run Nox as a regular user. The system will ask for your password when needed."));
        return 1;
    }
#endif
    QString directory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(directory);
    QLockFile lock(directory + "/nox.lock");
    if (!smokeTest && !lock.tryLock(0)) {
        QMessageBox::information(nullptr, "Nox", trText("Nox is already running in another window."));
        return 1;
    }

    Window window;
    window.show();

    if (smokeTest) {
        // Check the window and search without installing anything.
        QTimer::singleShot(0, &app, [&app, &window]() {
            QLineEdit *query = window.findChild<QLineEdit *>("search");
            QPushButton *node = window.findChild<QPushButton *>("install_node");
            QPushButton *go = window.findChild<QPushButton *>("install_go");
            if (!query || !node || !go) {
                app.exit(1);
                return;
            }
            query->setText("Go");
            bool correct = go->isVisible() && !node->isVisible();
            query->clear();
            correct = correct && node->isVisible();
            app.exit(correct ? 0 : 1);
        });
    }
    int screenshot = app.arguments().indexOf("--screenshot");
    if (screenshot > 0 && screenshot + 1 < app.arguments().size()) {
        QString path = app.arguments()[screenshot + 1];
        QTimer::singleShot(500, &app, [&app, &window, path]() {
            app.exit(window.grab().save(path) ? 0 : 1);
        });
    }
    return app.exec();
}
