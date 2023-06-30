#include "mainwindow.h"
#include "packagewindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    MainWindow w;
    PackageWindow w;
    w.show();
    return a.exec();
}
