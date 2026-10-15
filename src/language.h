#pragma once

#include <QString>

void loadLanguage();
void setLanguage(const QString &language, bool save = true);
QString currentLanguage();
QString trText(const char *english);
