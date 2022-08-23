#include "mainwindow.h"
#include <QApplication>

void standaloneTest();

int main(int argc, char *argv[])
{
#if 0
    QApplication a(argc, argv);
    auto w = std::make_shared<MainWindow>();
    w->show();
    return a.exec();
#else
    standaloneTest();
#endif
}
