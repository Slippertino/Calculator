#include "mainwindow.hpp"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QKeyEvent>
#include <QClipboard>
#include <QFile>

MainWindow::MainWindow(Presenter &presenter, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , presenter_(presenter)
    , translations_{ "ru", "en" }
    , currentTranslation_{ -1 }
{
    ui->setupUi(this);
    edit_ = ui->editExpression;
    edit_->installEventFilter(this);
    edit_->setFocusPolicy(Qt::StrongFocus);
    presenter_.setStatusMetrics(QFontMetrics{ ui->statusLabel->font()  });

    changeLanguage();

    QObject::connect(
        edit_, &QLineEdit::selectionChanged,
        this, [&]() {
            selection_ = {
                edit_->selectionStart(),
                edit_->selectionEnd()
            };

            auto& sel = selection_.value();
            if (sel.first == -1 || sel.second == -1)
                selection_ = std::nullopt;
        }
    );

    QObject::connect(
        edit_, &QLineEdit::cursorPositionChanged,
        this, [&](int, int) {
            presenter_.setPosition(
                edit_->cursorPosition()
            );
        }
    );

    addTrivial(ui->push0,       QString::fromStdWString(L"0"));
    addTrivial(ui->push1,       QString::fromStdWString(L"1"));
    addTrivial(ui->push2,       QString::fromStdWString(L"2"));
    addTrivial(ui->push3,       QString::fromStdWString(L"3"));
    addTrivial(ui->push4,       QString::fromStdWString(L"4"));
    addTrivial(ui->push5,       QString::fromStdWString(L"5"));
    addTrivial(ui->push6,       QString::fromStdWString(L"6"));
    addTrivial(ui->push7,       QString::fromStdWString(L"7"));
    addTrivial(ui->push8,       QString::fromStdWString(L"8"));
    addTrivial(ui->push9,       QString::fromStdWString(L"9"));
    addTrivial(ui->pushAdd,     QString::fromStdWString(L"+"));
    addTrivial(ui->pushMinus,   QString::fromStdWString(L"-"));
    addTrivial(ui->pushMult,    QString::fromStdWString(L"x"));
    addTrivial(ui->pushDiv,     QString::fromStdWString(L"/"));
    addTrivial(ui->pushPow,     QString::fromStdWString(L"^"));
    addTrivial(ui->pushFact,    QString::fromStdWString(L"!"));
    addTrivial(ui->pushSqrt,    QString::fromStdWString(L"\u221A"));
    addTrivial(ui->pushMod,     QString::fromStdWString(L"%"));
    addTrivial(ui->pushLg,      QString::fromStdWString(L"lg"));
    addTrivial(ui->pushLn,      QString::fromStdWString(L"ln"));
    addTrivial(ui->pushPi,      QString::fromStdWString(L"\u03C0"));
    addTrivial(ui->pushE,       QString::fromStdWString(L"\u03B5"));
    addTrivial(ui->pushDot,     QString::fromStdWString(L"."));
    addTrivial(ui->pushCos,     QString::fromStdWString(L"cos"));
    addTrivial(ui->pushSin,     QString::fromStdWString(L"sin"));
    addTrivial(ui->pushTan,     QString::fromStdWString(L"tan"));
    addTrivial(ui->pushAcos,    QString::fromStdWString(L"acos"));
    addTrivial(ui->pushAsin,    QString::fromStdWString(L"asin"));
    addTrivial(ui->pushAtan,    QString::fromStdWString(L"atan"));

    addComplex(ui->pushBracket, [&]() { presenter_.onBracket();});
    addComplex(ui->pushClear,   [&]() { presenter_.onClear();});
    addComplex(ui->pushFract,   [&]() { presenter_.onFraction();});
    addComplex(ui->pushInvert,  [&]() { presenter_.onInvert();});
    
    QObject::connect(
        ui->pushEval, &QPushButton::clicked, 
        this, [&] { handleEval(); }
    );

    loadStyles();
}

QSize MainWindow::sizeHint() const  {
    return QSize{ 647, 547 };
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);
}

int MainWindow::getSelectionRemove() const {
    if (selection_ == std::nullopt)
        return 0;
    auto sz = selection_->second - selection_->first;
    if (selection_->second == edit_->cursorPosition())
        sz *= -1;
    return sz;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == edit_ && event->type() == QEvent::Resize) 
        presenter_.setStatusWidth(edit_->width());

    if (obj != edit_ || event->type() != QEvent::KeyPress)
        return QMainWindow::eventFilter(obj, event);

    auto kevent = static_cast<QKeyEvent*>(event);
    auto key = kevent->key();
    switch (key) {
    case Qt::Key_Delete:
        presenter_.onRemove(
            (selection_ != std::nullopt)
                ? getSelectionRemove()
                : 1
        );
        break;
    case Qt::Key_Backspace:
        presenter_.onRemove(
            (selection_ != std::nullopt)
                ? getSelectionRemove()
                : -1
        );
        break;
    case Qt::Key_Return:
        [[fallthrough]];
    case Qt::Key_Enter:
        handleEval();
        return true;
    case Qt::Key_Alt:
        if (kevent->modifiers() & Qt::ShiftModifier)
            changeLanguage();
        return false;
    case Qt::Key_V:
        if (kevent->modifiers() & Qt::ControlModifier) {
            auto cb = QApplication::clipboard();
            auto upd = cb->text(QClipboard::Clipboard);
            presenter_.onInsert(upd);
            break;
        }
        [[fallthrough]];
    case Qt::Key_A:
        if (kevent->modifiers() & Qt::ControlModifier)
            return false;
        [[fallthrough]];
    default: {
        if (key < 0x01000000) {
            auto text = kevent->text();
            if (text.size() == 1 && symbolBindings_.contains(text.front()))
                text = QString(1, symbolBindings_[text.front()]);
            if (!text.isEmpty())
                presenter_.onInsert(text);
            break;
        }

        return false;
    }
    }
    update();

    return true;
}

void MainWindow::changeLanguage() {
    currentTranslation_ = (currentTranslation_ + 1) % translations_.size();
    translator_.load(
        QString("%1_%2")
                .arg(TRANSLATION_PREFIX)
                .arg(translations_[currentTranslation_])
    );
    qApp->installTranslator(&translator_);
}

void MainWindow::update() {
    edit_->setText(presenter_.getText());
    edit_->setCursorPosition(presenter_.getCursor());
    edit_->setFocus();
    ui->statusLabel->setText(presenter_.getStatus());
}

void MainWindow::handleEval() {
    auto st = presenter_.onEval();
    update();
    ui->statusLabel->setText(st);
}

void MainWindow::addTrivial(QPushButton* btn, const QString &op) {
    QObject::connect(
        btn, &QPushButton::clicked,
        this, [&, op](){
            presenter_.onInsert(op);
            update();
        }
    );
}

void MainWindow::loadStyles() {
    QFile file{":/resources/styles.qss"};
    file.open(QFile::ReadOnly);
    if (file.isOpen()){
        auto str = file.readAll();
        setStyleSheet(str);
        file.close();
    } else {
        qWarning() << "Failed to upload the app stylesheet\n";
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
