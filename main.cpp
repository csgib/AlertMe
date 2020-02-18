#include "mainwindow.h"

#include <QApplication>
#include <QtNetwork>
#include <QTcpServer>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    MainWindow w;
    w.setWindowFlags ( Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    w.hide();

    QTcpServer mytcp;
    QTcpSocket mysocket;

    return a.exec();
}
