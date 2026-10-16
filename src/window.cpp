#include "language.h"
#include "theme.h"
#include "window.h"

#include <QCloseEvent>
#include <QButtonGroup>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QShortcut>
#include <QStyle>
#include <QSvgRenderer>
#include <QTextCursor>
#include <QVBoxLayout>

Window::Window(QWidget *parent) : QMainWindow(parent), systemInfo(detectSystem()), installer(this) {
    setWindowTitle("Nox");
    setWindowIcon(QIcon(":/nox.svg"));
    resize(1080, 880);
    setMinimumSize(820, 640);
    buildUi();

    connect(&installer, &Installer::message, this, &Window::appendLog);
    connect(&installer, &Installer::finished, this, [this](bool success, const QString &text) {
        setBusy(false);
        message->setText(success ? trText("Installation completed.") : trText("Installation failed. Check the log."));
        appendLog("\n" + text + "\n");
        refreshDetection();
    });
}

void Window::buildUi() {
    QWidget *content = new QWidget;
    setCentralWidget(content);
    QVBoxLayout *page = new QVBoxLayout(content);
    page->setContentsMargins(24, 20, 24, 20);
    page->setSpacing(22);

    QHBoxLayout *header = new QHBoxLayout;
    QLabel *brandIcon = new QLabel;
    brandIcon->setPixmap(QIcon(":/nox.svg").pixmap(44, 44));
    brandIcon->setFixedSize(44, 44);
    header->addWidget(brandIcon);
    QVBoxLayout *brandText = new QVBoxLayout;
    brandText->setSpacing(0);
    QLabel *brand = new QLabel("nox");
    brand->setObjectName("brand");
    brandText->addWidget(brand);
    QLabel *tagline = new QLabel(trText("DEVELOPER WORKSPACE"));
    tagline->setObjectName("eyebrow");
    brandText->addWidget(tagline);
    header->addLayout(brandText);
    header->addStretch();
    QLabel *systemLabel = new QLabel(systemInfo.name + " / " + systemInfo.architecture);
    systemLabel->setObjectName("system");
    header->addWidget(systemLabel, 0, Qt::AlignVCenter);
    settingsButton = new QPushButton(trText("Settings"));
    settingsButton->setObjectName("settings");
    header->addWidget(settingsButton);
    connect(settingsButton, &QPushButton::clicked, this, &Window::openSettings);
    page->addLayout(header);

    QHBoxLayout *workspace = new QHBoxLayout;
    workspace->setSpacing(24);
    QFrame *sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(178);
    QVBoxLayout *navigation = new QVBoxLayout(sidebar);
    navigation->setContentsMargins(12, 20, 12, 16);
    navigation->setSpacing(7);
    QLabel *browse = new QLabel(trText("WORKSPACE"));
    browse->setObjectName("sectionLabel");
    navigation->addWidget(browse);
    navigation->addSpacing(8);
    QButtonGroup *categories = new QButtonGroup(sidebar);
    const QList<QPair<QString, QString>> categoryNames = {
        {"all", trText("All tools")}, {"languages", trText("Languages")},
        {"editors", trText("Editors")}, {"containers", trText("Containers")},
        {"compilers", trText("Compilers")}
    };
    for (const auto &category : categoryNames) {
        QPushButton *button = new QPushButton(category.second);
        button->setObjectName("category_" + category.first);
        button->setProperty("nav", true);
        button->setCheckable(true);
        categories->addButton(button);
        button->setChecked(selectedCategory == category.first);
        navigation->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, category]() {
            selectedCategory = category.first;
            filterTools(searchField->text());
        });
    }
    navigation->addStretch();
    QLabel *appearance = new QLabel(trText("APPEARANCE"));
    appearance->setObjectName("sectionLabel");
    navigation->addWidget(appearance);
    themePicker = new QComboBox;
    themePicker->setObjectName("themePicker");
    themePicker->setAccessibleName(trText("Theme"));
    for (const Theme &theme : availableThemes()) {
        QPixmap swatch(12, 12);
        swatch.fill(QColor(theme.accent));
        themePicker->addItem(QIcon(swatch), trText(theme.name), theme.id);
    }
    themePicker->setCurrentIndex(themePicker->findData(currentTheme()));
    connect(themePicker, &QComboBox::currentIndexChanged, this, [this]() {
        setTheme(themePicker->currentData().toString());
    });
    navigation->addWidget(themePicker);
    navigation->addSpacing(12);
    QLabel *sidebarNote = new QLabel(trText("A little setup.\nA lot of possibility."));
    sidebarNote->setObjectName("footnote");
    sidebarNote->setWordWrap(true);
    navigation->addWidget(sidebarNote);
    workspace->addWidget(sidebar);

    QVBoxLayout *catalog = new QVBoxLayout;
    catalog->setSpacing(14);
    QLabel *heading = new QLabel(trText("Make room for your next idea."));
    heading->setObjectName("heading");
    heading->setWordWrap(true);
    catalog->addWidget(heading);
    QLabel *subtitle = new QLabel(trText("Your essential dev tools. One place to get started."));
    subtitle->setObjectName("subtitle");
    subtitle->setWordWrap(true);
    catalog->addWidget(subtitle);
    catalog->addSpacing(4);

    QHBoxLayout *toolbar = new QHBoxLayout;
    searchField = new QLineEdit;
    searchField->setObjectName("search");
    searchField->setPlaceholderText(trText("Search tools…"));
    searchField->setClearButtonEnabled(true);
    searchField->setAccessibleName(trText("Search tools"));
    toolbar->addWidget(searchField);
    refreshButton = new QPushButton(trText("Check installed"));
    toolbar->addWidget(refreshButton);
    catalog->addLayout(toolbar);
    connect(searchField, &QLineEdit::textChanged, this, &Window::filterTools);
    connect(refreshButton, &QPushButton::clicked, this, &Window::refreshDetection);
    QShortcut *searchShortcut = new QShortcut(QKeySequence("Ctrl+K"), content);
    connect(searchShortcut, &QShortcut::activated, searchField, qOverload<>(&QWidget::setFocus));

    QHBoxLayout *catalogBar = new QHBoxLayout;
    QLabel *catalogTitle = new QLabel(trText("Tool library"));
    catalogTitle->setObjectName("catalogTitle");
    catalogBar->addWidget(catalogTitle);
    catalogBar->addStretch();
    toolCount = new QLabel;
    toolCount->setObjectName("toolCount");
    catalogBar->addWidget(toolCount);
    catalog->addLayout(catalogBar);

    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    QWidget *list = new QWidget;
    QVBoxLayout *rows = new QVBoxLayout(list);
    rows->setContentsMargins(0, 0, 0, 0);
    rows->setSpacing(8);
    rows->addWidget(createRow("node", "Node.js LTS", trText("JavaScript runtime. Includes npm.")));
    rows->addWidget(createRow("npm", "npm", trText("Package manager. Installs with Node.js LTS.")));
    rows->addWidget(createRow("go", "Go", trText("Latest stable release.")));
    rows->addWidget(createRow("zed", "Zed", trText("Code editor.")));
    rows->addWidget(createRow("docker", "Docker", trText("Desktop or command-line Engine.")));
    rows->addWidget(createRow("c", "C", trText("GCC compiler.")));
    rows->addWidget(createRow("cpp", "C++", trText("G++ compiler.")));
    rows->addStretch();
    scroll->setWidget(list);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    catalog->addWidget(scroll, 1);
    noResults = new QLabel(trText("No tools found."));
    noResults->setAlignment(Qt::AlignCenter);
    noResults->hide();
    catalog->addWidget(noResults);

    QHBoxLayout *logBar = new QHBoxLayout;
    message = new QLabel(trText("Installation log"));
    message->setObjectName("message");
    message->setWordWrap(true);
    logBar->addWidget(message, 1);
    QPushButton *save = new QPushButton(trText("Save log"));
    logBar->addWidget(save);
    connect(save, &QPushButton::clicked, this, &Window::saveLog);
    catalog->addLayout(logBar);

    progress = new QProgressBar;
    progress->setRange(0, 0);
    progress->setTextVisible(false);
    progress->setFixedHeight(4);
    progress->hide();
    catalog->addWidget(progress);
    log = new QPlainTextEdit;
    log->setObjectName("log");
    log->setReadOnly(true);
    log->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    log->setMaximumBlockCount(3000);
    log->setFixedHeight(76);
    log->setPlaceholderText(trText("No installations in this session."));
    catalog->addWidget(log);
    workspace->addLayout(catalog, 1);
    page->addLayout(workspace, 1);

    refreshDetection();
    filterTools(searchField->text());
}

void Window::openSettings() {
    if (installer.busy())
        return;

    QDialog dialog(this);
    dialog.setWindowTitle(trText("Settings"));
    dialog.setMinimumWidth(380);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    QLabel *settingsTitle = new QLabel(trText("Make Nox yours."));
    settingsTitle->setObjectName("heading");
    layout->addWidget(settingsTitle);
    QLabel *themeLabel = new QLabel(trText("Theme"));
    layout->addWidget(themeLabel);
    QComboBox *theme = new QComboBox;
    theme->setObjectName("theme");
    theme->setAccessibleName(trText("Theme"));
    for (const Theme &option : availableThemes())
        theme->addItem(trText(option.name), option.id);
    const QString previousTheme = currentTheme();
    theme->setCurrentIndex(theme->findData(previousTheme));
    themeLabel->setBuddy(theme);
    layout->addWidget(theme);
    QLabel *previewNote = new QLabel(trText("Preview a theme. Save it when it feels right."));
    previewNote->setObjectName("description");
    previewNote->setWordWrap(true);
    layout->addWidget(previewNote);
    connect(theme, &QComboBox::currentIndexChanged, &dialog, [theme]() {
        setTheme(theme->currentData().toString(), false);
    });
    QLabel *label = new QLabel(trText("Language"));
    layout->addWidget(label);
    QComboBox *language = new QComboBox;
    language->setObjectName("language");
    language->setAccessibleName(trText("Language"));
    language->addItem("English", "en");
    language->addItem(QString::fromUtf8("Português"), "pt_BR");
    language->setCurrentIndex(language->findData(currentLanguage()));
    label->setBuddy(language);
    layout->addWidget(language);

    QHBoxLayout *buttons = new QHBoxLayout;
    QPushButton *cancel = new QPushButton(trText("Cancel"));
    cancel->setObjectName("cancelSettings");
    QPushButton *save = new QPushButton(trText("Save"));
    save->setObjectName("saveSettings");
    save->setDefault(true);
    save->setProperty("primary", true);
    buttons->addWidget(cancel);
    buttons->addWidget(save);
    layout->addLayout(buttons);
    connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(save, &QPushButton::clicked, &dialog, &QDialog::accept);
    if (dialog.exec() != QDialog::Accepted) {
        setTheme(previousTheme, false);
        return;
    }
    setTheme(theme->currentData().toString());
    themePicker->setCurrentIndex(themePicker->findData(currentTheme()));
    if (language->currentData().toString() == currentLanguage())
        return;

    setLanguage(language->currentData().toString());
    QString savedLog = log->toPlainText();
    QString savedSearch = searchField->text();
    QWidget *previous = takeCentralWidget();
    previous->hide();
    previous->setParent(nullptr);
    previous->deleteLater();
    tools.clear();
    statuses.clear();
    installButtons.clear();
    buildUi();
    log->setPlainText(savedLog);
    searchField->setText(savedSearch);
}

QWidget *Window::createRow(const QString &id, const QString &name, const QString &description) {
    QFrame *line = new QFrame;
    line->setObjectName("tool");
    line->setProperty("search", name + " " + description);
    const QString category = (id == "c" || id == "cpp") ? "compilers"
        : id == "docker" ? "containers" : id == "zed" ? "editors" : "languages";
    line->setProperty("category", category);
    line->setMinimumHeight(76);
    QHBoxLayout *layout = new QHBoxLayout(line);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(14);

    // SVGs are embedded in the executable and work offline.
    QSvgRenderer drawing(":/logos/" + id + ".svg");
    QSize size = drawing.defaultSize();
    size.scale(30, 30, Qt::KeepAspectRatio);
    QPixmap image(size * devicePixelRatioF());
    image.setDevicePixelRatio(devicePixelRatioF());
    image.fill(Qt::transparent);
    QPainter painter(&image);
    drawing.render(&painter, QRectF(QPointF(0, 0), size));
    painter.end();
    QLabel *icon = new QLabel;
    icon->setObjectName("toolIcon");
    icon->setProperty("dark", id == "zed");
    icon->setPixmap(image);
    icon->setAlignment(Qt::AlignCenter);
    icon->setFixedSize(48, 48);
    icon->setAccessibleName(trText("Logo for ") + name);
    layout->addWidget(icon);

    QVBoxLayout *texts = new QVBoxLayout;
    texts->setSpacing(3);
    QLabel *title = new QLabel(name);
    title->setObjectName("toolName");
    texts->addWidget(title);
    QLabel *text = new QLabel(description);
    text->setObjectName("description");
    text->setWordWrap(true);
    texts->addWidget(text);
    layout->addLayout(texts, 1);
    QLabel *status = new QLabel;
    status->setObjectName("status");
    statuses[id] = status;
    status->setAlignment(Qt::AlignCenter);
    status->setMinimumWidth(72);
    layout->addWidget(status, 0, Qt::AlignVCenter);
    QPushButton *button = new QPushButton(id == "docker" ? trText("Choose…") : trText("Install"));
    button->setObjectName("install_" + id);
    button->setProperty("primary", true);
    button->setAccessibleName(trText("Install ") + name);
    button->setFixedWidth(94);
    layout->addWidget(button);
    installButtons.append(button);
    connect(button, &QPushButton::clicked, this, [this, id]() {
        if (id == "docker")
            chooseDocker();
        else
            confirmInstallation(id);
    });
    tools[id] = line;
    return line;
}

void Window::chooseDocker() {
    QDialog dialog(this);
    dialog.setWindowTitle("Docker");
    dialog.setMinimumWidth(480);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(18);
    QLabel *title = new QLabel(trText("Installation type"));
    title->setObjectName("toolName");
    layout->addWidget(title);
    QRadioButton *engine = new QRadioButton(trText("Docker Engine\nDocker service and command-line tools."));
    QRadioButton *desktop = new QRadioButton(trText("Docker Desktop\nGraphical interface for containers and images."));
    engine->setObjectName("dockerEngine");
    desktop->setObjectName("dockerDesktop");
    engine->setChecked(systemInfo.family != "windows");
    desktop->setChecked(systemInfo.family == "windows");
    layout->addWidget(engine);
    layout->addWidget(desktop);
    QPushButton *next = new QPushButton(trText("Continue"));
    layout->addWidget(next);
    connect(next, &QPushButton::clicked, &dialog, &QDialog::accept);
    if (dialog.exec() == QDialog::Accepted)
        confirmInstallation(engine->isChecked() ? "docker-engine" : "docker-desktop");
}

void Window::confirmInstallation(const QString &id) {
    if (installer.busy())
        return;
    InstallPlan plan = createPlan(id, systemInfo);
    if (!plan.error.isEmpty()) {
        QMessageBox::information(this, plan.title, plan.error);
        return;
    }
    QDialog dialog(this);
    dialog.setWindowTitle(trText("Install ") + plan.title);
    dialog.resize(650, 480);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    QLabel *title = new QLabel(plan.title);
    title->setObjectName("toolName");
    layout->addWidget(title);
    QLabel *description = new QLabel(plan.description);
    description->setTextFormat(Qt::PlainText);
    description->setWordWrap(true);
    layout->addWidget(description);
    layout->addWidget(new QLabel(trText("Commands:")));
    QPlainTextEdit *commands = new QPlainTextEdit(plan.commands);
    commands->setReadOnly(true);
    commands->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(commands);
    QHBoxLayout *buttons = new QHBoxLayout;
    QPushButton *cancel = new QPushButton(trText("Cancel"));
    QPushButton *confirm = new QPushButton(trText("Install ") + plan.title);
    buttons->addWidget(cancel);
    buttons->addWidget(confirm);
    layout->addLayout(buttons);
    connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(confirm, &QPushButton::clicked, &dialog, &QDialog::accept);
    if (dialog.exec() != QDialog::Accepted)
        return;
    appendLog("\n[" + QDateTime::currentDateTime().toString("HH:mm:ss") + trText("] Installing ") + plan.title + "\n");
    setBusy(true);
    message->setText(trText("Installing ") + plan.title + "…");
    installer.install(plan, systemInfo);
}

void Window::refreshDetection() {
    QMap<QString, QString> executables = {{"node", "node"}, {"npm", "npm"}, {"go", "go"},
        {"zed", "zed"}, {"c", "gcc"}, {"cpp", "g++"}, {"docker", "docker"}};
#ifdef Q_OS_WIN
    executables["npm"] = "npm.cmd";
#endif
    for (auto item = executables.cbegin(); item != executables.cend(); ++item) {
        QString path = findExecutable(item.value());
        statuses[item.key()]->setText(path.isEmpty() ? trText("Not found") : trText("Detected"));
        statuses[item.key()]->setToolTip(path);
        statuses[item.key()]->setProperty("detected", !path.isEmpty());
        statuses[item.key()]->style()->unpolish(statuses[item.key()]);
        statuses[item.key()]->style()->polish(statuses[item.key()]);
    }
    QString docker = findExecutable("docker");
    statuses["docker"]->setText(docker.isEmpty() ? trText("Not found") : trText("CLI detected"));
    statuses["docker"]->setToolTip(trText("Finding the CLI does not confirm that the service is running.\n") + docker);
    updateToolCount();
}

void Window::updateToolCount() {
    int detected = 0;
    int visible = 0;
    for (auto tool = tools.cbegin(); tool != tools.cend(); ++tool) {
        if (!tool.value()->isHidden()) {
            ++visible;
            if (statuses[tool.key()]->property("detected").toBool())
                ++detected;
        }
    }
    toolCount->setText(QString::number(visible) + (visible == 1 ? trText(" tool") : trText(" tools"))
        + "  /  " + QString::number(detected) + trText(" detected"));
}

void Window::filterTools(const QString &query) {
    int found = 0;
    for (QWidget *tool : tools) {
        bool show = tool->property("search").toString().contains(query.trimmed(), Qt::CaseInsensitive)
            && (selectedCategory == "all" || tool->property("category").toString() == selectedCategory);
        tool->setVisible(show);
        if (show)
            found++;
    }
    noResults->setVisible(found == 0);
    updateToolCount();
}

void Window::appendLog(const QString &text) {
    log->moveCursor(QTextCursor::End);
    log->insertPlainText(text);
    log->ensureCursorVisible();
}

void Window::setBusy(bool busy) {
    for (QPushButton *button : installButtons)
        button->setEnabled(!busy);
    refreshButton->setEnabled(!busy);
    settingsButton->setEnabled(!busy);
    progress->setVisible(busy);
}

void Window::saveLog() {
    QString path = QFileDialog::getSaveFileName(this, trText("Save log"), "nox-log.txt", trText("Text (*.txt)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(log->toPlainText().toUtf8()) < 0)
        QMessageBox::warning(this, trText("Log"), trText("Could not save the log."));
}

void Window::closeEvent(QCloseEvent *event) {
    if (installer.busy()) {
        QMessageBox::information(this, trText("Installation in progress"), trText("Wait for the installation to finish before closing Nox."));
        event->ignore();
        return;
    }
    event->accept();
}
