#include "../src/language.h"
#include "../src/system.h"
#include "../src/window.h"
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class WindowTest : public QObject {
    Q_OBJECT
private slots:
    void languageSettings();
};

void WindowTest::languageSettings() {
    QTemporaryDir settingsFolder;
    QVERIFY(settingsFolder.isValid());
    QCoreApplication::setOrganizationName("NoxTests");
    QCoreApplication::setApplicationName("window");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsFolder.path());
    loadLanguage();
    QCOMPARE(currentLanguage(), QString("en"));

    Window window;
    window.show();
    QCOMPARE(window.findChild<QPushButton *>("settings")->text(), QString("Settings"));
    QCOMPARE(window.findChild<QPushButton *>("install_node")->text(), QString("Install"));
    window.findChild<QPlainTextEdit *>("log")->setPlainText("Existing log\n");
    window.findChild<QLineEdit *>("search")->setText("Go");

    // Change the language through the same dialog used by the user.
    QTimer::singleShot(0, &window, []() {
        QDialog *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        QComboBox *language = dialog->findChild<QComboBox *>("language");
        QVERIFY(language);
        language->setCurrentIndex(language->findData("pt_BR"));
        dialog->findChild<QPushButton *>("saveSettings")->click();
    });
    window.findChild<QPushButton *>("settings")->click();
    QCOMPARE(currentLanguage(), QString("pt_BR"));
    QCOMPARE(window.findChild<QPushButton *>("settings")->text(), QString::fromUtf8("Configurações"));
    QCOMPARE(window.findChild<QPushButton *>("install_node")->text(), QString("Instalar"));
    QCOMPARE(window.findChild<QLineEdit *>("search")->text(), QString("Go"));
    QCOMPARE(window.findChild<QPlainTextEdit *>("log")->toPlainText(), QString("Existing log\n"));
    QVERIFY(window.findChild<QPushButton *>("install_node")->isHidden() ||
            !window.findChild<QPushButton *>("install_node")->isVisibleTo(&window));

    SystemInfo unsupported;
    QCOMPARE(createPlan("node", unsupported).error, QString::fromUtf8("Esta distribuição ainda não tem um instalador disponível."));

    // Cancelling must leave both the current and saved language unchanged.
    QTimer::singleShot(0, &window, []() {
        QDialog *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        QComboBox *language = dialog->findChild<QComboBox *>("language");
        QCOMPARE(language->currentData().toString(), QString("pt_BR"));
        language->setCurrentIndex(language->findData("en"));
        dialog->findChild<QPushButton *>("cancelSettings")->click();
    });
    window.findChild<QPushButton *>("settings")->click();
    QCOMPARE(currentLanguage(), QString("pt_BR"));
    setLanguage("en", false);
    loadLanguage();
    QCOMPARE(currentLanguage(), QString("pt_BR"));
    Window reopened;
    QCOMPARE(reopened.findChild<QPushButton *>("settings")->text(), QString::fromUtf8("Configurações"));

    QTimer::singleShot(0, &window, []() {
        QDialog *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        QComboBox *language = dialog->findChild<QComboBox *>("language");
        language->setCurrentIndex(language->findData("en"));
        dialog->findChild<QPushButton *>("saveSettings")->click();
    });
    window.findChild<QPushButton *>("settings")->click();
    QCOMPARE(window.findChild<QPushButton *>("settings")->text(), QString("Settings"));
    setLanguage("pt_BR", false);
    loadLanguage();
    QCOMPARE(currentLanguage(), QString("en"));
    QCOMPARE(createPlan("node", unsupported).error, QString("This distribution is not supported yet."));
    QCOMPARE(trText("Untranslated text"), QString("Untranslated text"));
}

QTEST_MAIN(WindowTest)
#include "window_test.moc"
