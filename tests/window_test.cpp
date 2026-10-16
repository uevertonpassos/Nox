#include "../src/language.h"
#include "../src/system.h"
#include "../src/window.h"
#include "../src/theme.h"
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class WindowTest : public QObject {
    Q_OBJECT
    QTemporaryDir settingsRoot;
private slots:
    void init();
    void languageSettings();
    void themeSettings();
    void categoriesAndSearch();
    void themesRender();
};

void WindowTest::init() {
    QVERIFY(settingsRoot.isValid());
    QCoreApplication::setOrganizationName("NoxTests");
    QCoreApplication::setApplicationName("window");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsRoot.path());
    QSettings().clear();
    qApp->setStyle("Fusion");
#ifdef Q_OS_WIN
    qApp->setFont(QFont("Segoe UI", 10));
#endif
    setLanguage("en", false);
    loadTheme();
}

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

void WindowTest::themeSettings() {
    Window window;
    window.show();
    auto *picker = window.findChild<QComboBox *>("themePicker");
    QCOMPARE(picker->count(), 5);
    picker->setCurrentIndex(picker->findData("ocean"));
    QCOMPARE(currentTheme(), QString("ocean"));
    setTheme("mint", false);
    loadTheme();
    QCOMPARE(currentTheme(), QString("ocean"));

    window.findChild<QLineEdit *>("search")->setText("Go");
    window.findChild<QPlainTextEdit *>("log")->setPlainText("Keep this output");
    QTimer::singleShot(0, &window, []() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        auto *theme = dialog->findChild<QComboBox *>("theme");
        theme->setCurrentIndex(theme->findData("violet"));
        QCOMPARE(currentTheme(), QString("violet"));
        QCOMPARE(QSettings().value("theme").toString(), QString("ocean"));
        dialog->reject();
    });
    window.findChild<QPushButton *>("settings")->click();
    QCOMPARE(currentTheme(), QString("ocean"));
    QCOMPARE(picker->currentData().toString(), QString("ocean"));

    QTimer::singleShot(0, &window, []() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        auto *theme = dialog->findChild<QComboBox *>("theme");
        theme->setCurrentIndex(theme->findData("daylight"));
        auto *language = dialog->findChild<QComboBox *>("language");
        language->setCurrentIndex(language->findData("pt_BR"));
        dialog->findChild<QPushButton *>("saveSettings")->click();
    });
    window.findChild<QPushButton *>("settings")->click();
    QCOMPARE(currentTheme(), QString("daylight"));
    QCOMPARE(currentLanguage(), QString("pt_BR"));
    QCOMPARE(window.findChild<QLineEdit *>("search")->text(), QString("Go"));
    QCOMPARE(window.findChild<QPlainTextEdit *>("log")->toPlainText(), QString("Keep this output"));
    setTheme("mint", false);
    loadTheme();
    Window reopened;
    QCOMPARE(reopened.findChild<QComboBox *>("themePicker")->currentData().toString(), QString("daylight"));
    setTheme("unknown-theme", false);
    QCOMPARE(currentTheme(), QString("mint"));
}

void WindowTest::categoriesAndSearch() {
    Window window;
    window.show();
    auto *query = window.findChild<QLineEdit *>("search");
    auto *node = window.findChild<QPushButton *>("install_node");
    auto *go = window.findChild<QPushButton *>("install_go");
    auto *zed = window.findChild<QPushButton *>("install_zed");
    window.findChild<QPushButton *>("category_languages")->click();
    QVERIFY(node->isVisibleTo(&window));
    QVERIFY(!zed->isVisibleTo(&window));
    query->setText(" Go ");
    QVERIFY(go->isVisibleTo(&window));
    QVERIFY(!node->isVisibleTo(&window));
    window.findChild<QPushButton *>("category_editors")->click();
    QVERIFY(!go->isVisibleTo(&window));
    QVERIFY(window.findChild<QLabel *>("toolCount")->text().startsWith("0 tools"));
    query->clear();
    QVERIFY(zed->isVisibleTo(&window));
    window.findChild<QPushButton *>("category_all")->click();
    QVERIFY(node->isVisibleTo(&window));
    QCOMPARE(window.findChild<QLabel *>("toolCount")->text().left(7), QString("7 tools"));
    // The smallest supported window must keep controls inside the viewport.
    window.resize(window.minimumSize());
    QCoreApplication::processEvents();
    QVERIFY(window.centralWidget()->rect().contains(query->mapTo(window.centralWidget(), QPoint(0, 0))));
}

void WindowTest::themesRender() {
    setLanguage("pt_BR", false);
    Window window;
    window.resize(1080, 1040);
    window.show();
    const QString screenshotDir = qEnvironmentVariable("NOX_SCREENSHOT_DIR");
    for (const Theme &theme : availableThemes()) {
        setTheme(theme.id, false);
        auto *picker = window.findChild<QComboBox *>("themePicker");
        picker->setCurrentIndex(picker->findData(theme.id));
        QCoreApplication::processEvents();
        // Check the rendered surface and control palette. Qt 6.2's global
        // application palette can report Fusion defaults while QSS is active.
        QCOMPARE(window.grab().toImage().pixelColor(2, 2), QColor(theme.background));
        QCOMPARE(window.findChild<QLineEdit *>("search")->palette().color(QPalette::Base), QColor(theme.field));
        QVERIFY(!qApp->styleSheet().contains('@'));
        QVERIFY(!window.windowIcon().isNull());
        if (!screenshotDir.isEmpty()) {
            QVERIFY(QDir().mkpath(screenshotDir));
            QVERIFY(window.grab().save(screenshotDir + "/nox-" + theme.id + ".png"));
        }
    }
}

QTEST_MAIN(WindowTest)
#include "window_test.moc"
