#include "language.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

static QString selectedLanguage = "en";
static QJsonObject translations;

void loadLanguage() {
    QSettings settings;
    setLanguage(settings.value("language", "en").toString(), false);
}

void setLanguage(const QString &language, bool save) {
    selectedLanguage = language == "pt_BR" ? "pt_BR" : "en";
    translations = QJsonObject();
    if (selectedLanguage == "pt_BR") {
        QFile file(":/translations/pt_BR.json");
        if (file.open(QIODevice::ReadOnly))
            translations = QJsonDocument::fromJson(file.readAll()).object();
    }
    if (save) {
        QSettings settings;
        settings.setValue("language", selectedLanguage);
    }
}

QString currentLanguage() {
    return selectedLanguage;
}

QString trText(const char *english) {
    QString source = QString::fromUtf8(english);
    return translations.value(source).toString(source);
}
