#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    MainWindow w;
    auto w = std::make_shared<MainWindow>();
    w->show();
    return a.exec();
}
