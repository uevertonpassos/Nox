#pragma once

#include <QString>
#include <QVector>

struct Theme {
    QString id;
    const char *name;
    QString background, surface, raised, field, border;
    QString text, muted, accent, accentText, success, successBackground;
};

const QVector<Theme> &availableThemes();
QString currentTheme();
void loadTheme();
void setTheme(const QString &id, bool save = true);
