#include "janela.h"

#include <QCloseEvent>
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
#include <QSvgRenderer>
#include <QTextCursor>
#include <QVBoxLayout>

Janela::Janela(QWidget *pai) : QMainWindow(pai), sistema(detectarSistema()), instalador(this) {
    setWindowTitle("Nox");
    setWindowIcon(QIcon(":/nox.svg"));
    resize(900, 790);
    setMinimumSize(740, 600);

    QWidget *conteudo = new QWidget;
    setCentralWidget(conteudo);
    QVBoxLayout *pagina = new QVBoxLayout(conteudo);
    pagina->setContentsMargins(20, 16, 20, 16);
    pagina->setSpacing(12);

    QHBoxLayout *cabecalho = new QHBoxLayout;
    QLabel *marca = new QLabel("Nox");
    marca->setObjectName("marca");
    cabecalho->addWidget(marca);
    cabecalho->addStretch();
    QLabel *sistemaDetectado = new QLabel(sistema.nome + " / " + sistema.arquitetura);
    sistemaDetectado->setObjectName("sistema");
    cabecalho->addWidget(sistemaDetectado);
    pagina->addLayout(cabecalho);

    QHBoxLayout *barra = new QHBoxLayout;
    campoBusca = new QLineEdit;
    campoBusca->setObjectName("busca");
    campoBusca->setPlaceholderText("Buscar ferramenta…");
    campoBusca->setClearButtonEnabled(true);
    campoBusca->setAccessibleName("Buscar ferramenta");
    barra->addWidget(campoBusca);
    botaoAtualizar = new QPushButton("Verificar instalados");
    barra->addWidget(botaoAtualizar);
    pagina->addLayout(barra);
    connect(campoBusca, &QLineEdit::textChanged, this, &Janela::filtrarFerramentas);
    connect(botaoAtualizar, &QPushButton::clicked, this, &Janela::atualizarDeteccao);
    QShortcut *atalhoBusca = new QShortcut(QKeySequence("Ctrl+K"), this);
    connect(atalhoBusca, &QShortcut::activated, campoBusca, qOverload<>(&QWidget::setFocus));

    QScrollArea *rolagem = new QScrollArea;
    rolagem->setWidgetResizable(true);
    rolagem->setFrameShape(QFrame::NoFrame);
    QWidget *lista = new QWidget;
    QVBoxLayout *linhas = new QVBoxLayout(lista);
    linhas->setContentsMargins(0, 0, 0, 0);
    linhas->setSpacing(0);
    linhas->addWidget(criarLinha("node", "Node.js LTS", "Runtime JavaScript. Inclui npm."));
    linhas->addWidget(criarLinha("npm", "npm", "Gerenciador de pacotes. Instala junto com Node.js LTS."));
    linhas->addWidget(criarLinha("go", "Go", "Versão estável mais recente."));
    linhas->addWidget(criarLinha("zed", "Zed", "Editor de código."));
    linhas->addWidget(criarLinha("docker", "Docker", "Desktop ou Engine pelo terminal."));
    linhas->addWidget(criarLinha("c", "C", "Compilador GCC."));
    linhas->addWidget(criarLinha("cpp", "C++", "Compilador G++."));
    linhas->addStretch();
    rolagem->setWidget(lista);
    pagina->addWidget(rolagem, 1);
    semResultados = new QLabel("Nenhuma ferramenta encontrada.");
    semResultados->setAlignment(Qt::AlignCenter);
    semResultados->hide();
    pagina->addWidget(semResultados);

    QHBoxLayout *barraRegistro = new QHBoxLayout;
    mensagem = new QLabel("Registro de instalação");
    mensagem->setObjectName("mensagem");
    mensagem->setWordWrap(true);
    barraRegistro->addWidget(mensagem, 1);
    QPushButton *salvar = new QPushButton("Salvar registro");
    barraRegistro->addWidget(salvar);
    connect(salvar, &QPushButton::clicked, this, &Janela::salvarRegistro);
    pagina->addLayout(barraRegistro);

    progresso = new QProgressBar;
    progresso->setRange(0, 0);
    progresso->setTextVisible(false);
    progresso->setFixedHeight(4);
    progresso->hide();
    pagina->addWidget(progresso);
    registro = new QPlainTextEdit;
    registro->setObjectName("registro");
    registro->setReadOnly(true);
    registro->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    registro->setMaximumBlockCount(3000);
    registro->setFixedHeight(85);
    registro->setPlaceholderText("Nenhuma instalação nesta sessão.");
    pagina->addWidget(registro);

    connect(&instalador, &Instalador::mensagem, this, &Janela::escreverRegistro);
    connect(&instalador, &Instalador::terminou, this, [this](bool sucesso, const QString &texto) {
        definirOcupado(false);
        mensagem->setText(sucesso ? "Instalação concluída." : "Falha na instalação. Consulte o registro.");
        escreverRegistro("\n" + texto + "\n");
        atualizarDeteccao();
    });
    atualizarDeteccao();
}

QWidget *Janela::criarLinha(const QString &id, const QString &nome, const QString &descricao) {
    QFrame *linha = new QFrame;
    linha->setObjectName("ferramenta");
    linha->setProperty("busca", nome + " " + descricao);
    linha->setMinimumHeight(74);
    QHBoxLayout *layout = new QHBoxLayout(linha);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(16);

    // Os SVGs ficam no executável; os ícones não precisam de internet.
    QSvgRenderer desenho(":/logos/" + id + ".svg");
    QSize tamanho = desenho.defaultSize();
    tamanho.scale(44, 40, Qt::KeepAspectRatio);
    QPixmap imagem(tamanho * devicePixelRatioF());
    imagem.setDevicePixelRatio(devicePixelRatioF());
    imagem.fill(Qt::transparent);
    QPainter pintor(&imagem);
    desenho.render(&pintor, QRectF(QPointF(0, 0), tamanho));
    pintor.end();
    QLabel *icone = new QLabel;
    icone->setPixmap(imagem);
    icone->setAlignment(Qt::AlignCenter);
    icone->setFixedSize(52, 44);
    icone->setAccessibleName("Logo de " + nome);
    layout->addWidget(icone);

    QVBoxLayout *textos = new QVBoxLayout;
    textos->setSpacing(3);
    QLabel *titulo = new QLabel(nome);
    titulo->setObjectName("nomeFerramenta");
    textos->addWidget(titulo);
    QLabel *texto = new QLabel(descricao);
    texto->setObjectName("descricao");
    texto->setWordWrap(true);
    textos->addWidget(texto);
    layout->addLayout(textos, 1);
    QLabel *situacao = new QLabel;
    situacao->setObjectName("situacao");
    situacoes[id] = situacao;
    situacao->setFixedWidth(130);
    layout->addWidget(situacao);
    QPushButton *botao = new QPushButton(id == "docker" ? "Escolher…" : "Instalar");
    botao->setObjectName("instalar_" + id);
    botao->setAccessibleName("Instalar " + nome);
    botao->setFixedWidth(110);
    layout->addWidget(botao);
    botoesInstalar.append(botao);
    connect(botao, &QPushButton::clicked, this, [this, id]() {
        if (id == "docker")
            escolherDocker();
        else
            confirmarInstalacao(id);
    });
    ferramentas[id] = linha;
    return linha;
}

void Janela::escolherDocker() {
    QDialog dialogo(this);
    dialogo.setWindowTitle("Docker");
    dialogo.setMinimumWidth(480);
    QVBoxLayout *layout = new QVBoxLayout(&dialogo);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(18);
    QLabel *titulo = new QLabel("Tipo de instalação");
    titulo->setObjectName("nomeFerramenta");
    layout->addWidget(titulo);
    QRadioButton *engine = new QRadioButton("Docker Engine\nServiço Docker e comandos pelo terminal.");
    QRadioButton *desktop = new QRadioButton("Docker Desktop\nInterface gráfica para containers e imagens.");
    engine->setObjectName("dockerEngine");
    desktop->setObjectName("dockerDesktop");
    engine->setChecked(sistema.familia != "windows");
    desktop->setChecked(sistema.familia == "windows");
    layout->addWidget(engine);
    layout->addWidget(desktop);
    QPushButton *continuar = new QPushButton("Continuar");
    layout->addWidget(continuar);
    connect(continuar, &QPushButton::clicked, &dialogo, &QDialog::accept);
    if (dialogo.exec() == QDialog::Accepted)
        confirmarInstalacao(engine->isChecked() ? "docker-engine" : "docker-desktop");
}

void Janela::confirmarInstalacao(const QString &id) {
    if (instalador.ocupado())
        return;
    Plano plano = criarPlano(id, sistema);
    if (!plano.erro.isEmpty()) {
        QMessageBox::information(this, plano.titulo, plano.erro);
        return;
    }
    QDialog dialogo(this);
    dialogo.setWindowTitle("Instalar " + plano.titulo);
    dialogo.resize(650, 480);
    QVBoxLayout *layout = new QVBoxLayout(&dialogo);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    QLabel *titulo = new QLabel(plano.titulo);
    titulo->setObjectName("nomeFerramenta");
    layout->addWidget(titulo);
    QLabel *descricao = new QLabel(plano.descricao);
    descricao->setTextFormat(Qt::PlainText);
    descricao->setWordWrap(true);
    layout->addWidget(descricao);
    layout->addWidget(new QLabel("Comandos:"));
    QPlainTextEdit *comandos = new QPlainTextEdit(plano.comandos);
    comandos->setReadOnly(true);
    comandos->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(comandos);
    QHBoxLayout *botoes = new QHBoxLayout;
    QPushButton *cancelar = new QPushButton("Cancelar");
    QPushButton *confirmar = new QPushButton("Instalar " + plano.titulo);
    botoes->addWidget(cancelar);
    botoes->addWidget(confirmar);
    layout->addLayout(botoes);
    connect(cancelar, &QPushButton::clicked, &dialogo, &QDialog::reject);
    connect(confirmar, &QPushButton::clicked, &dialogo, &QDialog::accept);
    if (dialogo.exec() != QDialog::Accepted)
        return;
    escreverRegistro("\n[" + QDateTime::currentDateTime().toString("HH:mm:ss") + "] Instalando " + plano.titulo + "\n");
    definirOcupado(true);
    mensagem->setText("Instalando " + plano.titulo + "…");
    instalador.instalar(plano, sistema);
}

void Janela::atualizarDeteccao() {
    QMap<QString, QString> executaveis = {{"node", "node"}, {"npm", "npm"}, {"go", "go"},
        {"zed", "zed"}, {"c", "gcc"}, {"cpp", "g++"}, {"docker", "docker"}};
#ifdef Q_OS_WIN
    executaveis["npm"] = "npm.cmd";
#endif
    for (auto item = executaveis.cbegin(); item != executaveis.cend(); ++item) {
        QString caminho = caminhoExecutavel(item.value());
        situacoes[item.key()]->setText(caminho.isEmpty() ? "Não encontrado" : "Detectado");
        situacoes[item.key()]->setToolTip(caminho);
    }
    QString docker = caminhoExecutavel("docker");
    situacoes["docker"]->setText(docker.isEmpty() ? "Não encontrado" : "CLI detectada");
    situacoes["docker"]->setToolTip("A detecção da CLI não confirma que o serviço está ativo.\n" + docker);
}

void Janela::filtrarFerramentas(const QString &busca) {
    int encontrados = 0;
    for (QWidget *ferramenta : ferramentas) {
        bool mostrar = ferramenta->property("busca").toString().contains(busca.trimmed(), Qt::CaseInsensitive);
        ferramenta->setVisible(mostrar);
        if (mostrar)
            encontrados++;
    }
    semResultados->setVisible(encontrados == 0);
}

void Janela::escreverRegistro(const QString &texto) {
    registro->moveCursor(QTextCursor::End);
    registro->insertPlainText(texto);
    registro->ensureCursorVisible();
}

void Janela::definirOcupado(bool ocupado) {
    for (QPushButton *botao : botoesInstalar)
        botao->setEnabled(!ocupado);
    botaoAtualizar->setEnabled(!ocupado);
    progresso->setVisible(ocupado);
}

void Janela::salvarRegistro() {
    QString caminho = QFileDialog::getSaveFileName(this, "Salvar registro", "nox-registro.txt", "Texto (*.txt)");
    if (caminho.isEmpty())
        return;
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::WriteOnly) || arquivo.write(registro->toPlainText().toUtf8()) < 0)
        QMessageBox::warning(this, "Registro", "Não foi possível salvar o registro.");
}

void Janela::closeEvent(QCloseEvent *evento) {
    if (instalador.ocupado()) {
        QMessageBox::information(this, "Instalação em andamento", "Aguarde a instalação terminar antes de fechar o Nox.");
        evento->ignore();
        return;
    }
    evento->accept();
}
