#pragma once

#include "system.h"
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>

class Installer : public QObject {
    Q_OBJECT
public:
    explicit Installer(QObject *parent = nullptr);
    void install(const InstallPlan &plan, const SystemInfo &systemInfo);
    bool busy() const;

signals:
    void message(const QString &text);
    void finished(bool success, const QString &text);

private:
    void fetch(const QString &url);
    void receiveReply();
    void run(const QString &program, const QStringList &arguments);
    void runToolchain();
    void finish(bool success, const QString &text);

    QProcess process;
    QNetworkAccessManager network;
    InstallPlan currentPlan;
    SystemInfo currentSystem;
    QString stage;
    QString version;
    QString file;
    QString packageUrl;
    QString checksum;
    bool working = false;
};
