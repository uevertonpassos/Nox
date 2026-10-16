#include "theme.h"

#include <QApplication>
#include <QFile>
#include <QMap>
#include <QPalette>
#include <QSettings>

static QString selectedTheme = "mint";

const QVector<Theme> &availableThemes() {
    static const QVector<Theme> themes = {
        {"mint", "Nox Mint", "#101715", "#18211e", "#22302a", "#111a16", "#304239",
         "#eef4f0", "#a0b5a9", "#b7ed8b", "#172510", "#b7ed8b", "#283b24"},
        {"ocean", "Ocean", "#101722", "#182231", "#233247", "#111a28", "#33465e",
         "#eef4fc", "#a0b5cf", "#8bc8ff", "#11253b", "#93dfbd", "#223b35"},
        {"violet", "Violet", "#191522", "#231e2e", "#322940", "#1b1626", "#493b59",
         "#f5effc", "#b9a8ce", "#cfb0ff", "#2c1745", "#a9dfb6", "#293c31"},
        {"ember", "Ember", "#201713", "#2b211b", "#3b2d23", "#221913", "#514032",
         "#fcf3ea", "#c7ae99", "#ffc18a", "#3b230e", "#bddf9a", "#343e28"},
        {"daylight", "Daylight", "#f3f5ef", "#ffffff", "#e9eee3", "#f8faf5", "#cdd6c5",
         "#202b1e", "#586650", "#365e23", "#ffffff", "#365e23", "#e7efdf"}
    };
    return themes;
}

QString currentTheme() { return selectedTheme; }

void loadTheme() {
    setTheme(QSettings().value("theme", "mint").toString(), false);
}

void setTheme(const QString &id, bool save) {
    const Theme *chosen = &availableThemes().first();
    for (const Theme &theme : availableThemes())
        if (theme.id == id) {
            chosen = &theme;
            break;
        }
    selectedTheme = chosen->id;
    const Theme &t = *chosen;

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(t.background));
    palette.setColor(QPalette::WindowText, QColor(t.text));
    palette.setColor(QPalette::Base, QColor(t.field));
    palette.setColor(QPalette::AlternateBase, QColor(t.surface));
    palette.setColor(QPalette::Text, QColor(t.text));
    palette.setColor(QPalette::Button, QColor(t.raised));
    palette.setColor(QPalette::ButtonText, QColor(t.text));
    palette.setColor(QPalette::Highlight, QColor(t.accent));
    palette.setColor(QPalette::HighlightedText, QColor(t.accentText));
    palette.setColor(QPalette::PlaceholderText, QColor(t.muted));
    palette.setColor(QPalette::ToolTipBase, QColor(t.raised));
    palette.setColor(QPalette::ToolTipText, QColor(t.text));
    palette.setColor(QPalette::Link, QColor(t.accent));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(t.muted));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(t.muted));

    QFile file(":/style.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QString sheet = QString::fromUtf8(file.readAll());
        const QMap<QString, QString> tokens = {
            {"background", t.background}, {"surface", t.surface}, {"raised", t.raised},
            {"field", t.field}, {"border", t.border}, {"text", t.text}, {"muted", t.muted},
            {"accentText", t.accentText}, {"accent", t.accent},
            {"successBackground", t.successBackground}, {"success", t.success}
        };
        for (auto token = tokens.cbegin(); token != tokens.cend(); ++token)
            sheet.replace("@" + token.key() + ";", token.value() + ";");
        for (auto token = tokens.cbegin(); token != tokens.cend(); ++token)
            sheet.replace("@" + token.key(), token.value());
        qApp->setStyleSheet(sheet);
    }
    // Supply base colors for controls not explicitly covered by the stylesheet.
    qApp->setPalette(palette);
    if (save)
        QSettings().setValue("theme", selectedTheme);
}
