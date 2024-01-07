#include <QApplication>
#include "view/mainwindow.hpp"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    Presenter presenter;
    MainWindow w{ presenter };
    w.show();
    return a.exec();
}