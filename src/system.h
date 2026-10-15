#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

struct SystemInfo {
    QString name;
    QString id;
    QString family;
    QString architecture;
    QString codename;
    bool immutable = false;
};

struct InstallPlan {
    QString tool;
    QString title;
    QString description;
    QString commands;
    QString error;
    QString program;
    QStringList arguments;
    bool administrator = false;
};

QMap<QString, QString> parseOsRelease(const QString &text);
SystemInfo identifyLinux(const QMap<QString, QString> &data);
SystemInfo detectSystem();
InstallPlan createPlan(const QString &tool, const SystemInfo &systemInfo);
QString findExecutable(const QString &name);
QString quoteArgument(const QString &text);
