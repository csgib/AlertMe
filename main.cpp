#include "mainwindow.h"

#include <QApplication>
#include <QtNetwork>
#include <QTcpServer>
#include <QStyleFactory>
#include <QLockFile>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QString tmpDir = QDir::tempPath();
    QLockFile lockFile(tmpDir + "/alertme.lock");

    if(!lockFile.tryLock(100))
    {
        return 1;
    }
    else
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
}
