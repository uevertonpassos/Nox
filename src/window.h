#pragma once

#include "installer.h"
#include "system.h"
#include <QMainWindow>
#include <QMap>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QComboBox;

class Window : public QMainWindow {
    Q_OBJECT
public:
    explicit Window(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void openSettings();
    QWidget *createRow(const QString &id, const QString &name, const QString &description);
    void chooseDocker();
    void confirmInstallation(const QString &id);
    void refreshDetection();
    void filterTools(const QString &query);
    void appendLog(const QString &text);
    void setBusy(bool busy);
    void saveLog();
    void updateToolCount();

    SystemInfo systemInfo;
    Installer installer;
    QMap<QString, QWidget *> tools;
    QMap<QString, QLabel *> statuses;
    QList<QPushButton *> installButtons;
    QLineEdit *searchField;
    QPlainTextEdit *log;
    QLabel *message;
    QLabel *noResults;
    QLabel *toolCount;
    QComboBox *themePicker;
    QString selectedCategory = "all";
    QPushButton *refreshButton;
    QPushButton *settingsButton;
    QProgressBar *progress;
};
