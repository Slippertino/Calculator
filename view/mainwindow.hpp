#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QHash>
#include "presenter/presenter.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Presenter &presenter, QWidget *parent = nullptr);
    ~MainWindow();

    QSize sizeHint() const override;

protected:
    void changeEvent(QEvent *event) override;

private:
    int getSelectionRemove() const;

    bool eventFilter(QObject *target, QEvent *event) override;
    void changeLanguage();
    void update();
    void handleEval();
    void addTrivial(QPushButton* btn, const QString &op);

    template<typename Handler>
    void addComplex(QPushButton* btn, Handler hndl) {
        QObject::connect(
            btn, &QPushButton::clicked,
            this, [&, hndl](){ hndl(); update(); }
        );
    }

    void loadStyles();

private:
    static inline QHash<QChar, QChar> symbolBindings_ = {
        { L'*',     L'x'      },
        { L'p',     L'\u03C0' },
        { L'e',     L'\u03B5' },
        { L'r',     L'\u221A' },
    };

private:
    Ui::MainWindow *ui;
    Presenter &presenter_;

    QLineEdit *edit_ = nullptr;
    std::optional<std::pair<int, int>> selection_ = std::nullopt;

    QStringList translations_;
    QTranslator translator_;
    int currentTranslation_;
};

#endif // MAINWINDOW_HPP
