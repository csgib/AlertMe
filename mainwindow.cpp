#include <QIODevice>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QNetworkInterface>
#include <QTimer>
#include <QMessageBox>
#include <QSound>
#include <QInputDialog>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "alert_dialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    wg_admin_pwd = "admin";

    sound_alarm = new QSound(":/new/prefix1/alarm.wav");
    sound_pinpon = new QSound(":/new/prefix1/alarm2.wav");

    m_alert_dialog = new alert_dialog();

    mytcpsrv = new QTcpServer();
    connect(mytcpsrv, SIGNAL(newConnection()),this, SLOT(new_srv_connect()));

    client_socket = new QTcpSocket();
    connect(client_socket, SIGNAL(connected()),this, SLOT(connected()));
    connect(client_socket, SIGNAL(disconnected()),this, SLOT(disconnected()));
    connect(client_socket, SIGNAL(bytesWritten(qint64)),this, SLOT(bytesWritten(qint64)));
    connect(client_socket, SIGNAL(readyRead()),this, SLOT(readyRead()));

    client_timer = new QTimer();
    client_timer->setInterval(2000);
    client_timer->setSingleShot(true);
    connect(client_timer, SIGNAL(timeout()), this, SLOT(timeout_client()));

    server_timer = new QTimer();
    server_timer->setInterval(2000);
    server_timer->setSingleShot(true);
    connect(server_timer, SIGNAL(timeout()), this, SLOT(timeout_server()));

    trayIcon = new QSystemTrayIcon(this);
    QIcon icon(":/new/prefix1/Images/icon.png");
    trayIcon->setIcon(icon);

    QMenu* stmenu = new QMenu(this);

    QAction* actTexte1 = new QAction("Afficher l'application",this);
    actTexte1->setIcon(QIcon(":/new/prefix1/Images/showico.png"));
    stmenu->addAction(actTexte1);
    connect(actTexte1, SIGNAL(triggered()), this, SLOT(display_interface()));

    trayIcon->setContextMenu(stmenu);
    trayIcon->show();

    ui->setupUi(this);

    // *** LECTURE FICHIER CONFIGURATION ***
    QFile file(QCoreApplication::applicationDirPath() + "/config.ini");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "FICHIER DE CONFIGURATION INTROUVABLE";
        wg_conf_host_data = "127.0.0.1";
        wg_local_port = "1976";
    }
    else
    {
        wg_local_port = "";
        wg_conf_host_data = "";

        while (!file.atEnd())
        {
            QByteArray line_conf = file.readLine();
            QString wl_conf_txt = QString::fromLatin1(line_conf);
            wl_conf_txt = wl_conf_txt.remove(QRegExp("[\\n\\t\\r]"));

            if ( wl_conf_txt.left(8) == "HST_DATA" )
            {
                wg_conf_host_data = wl_conf_txt.right(wl_conf_txt.length()-9);
            }

            if ( wl_conf_txt.left(8) == "SRV_PORT" )
            {
                wg_local_port = wl_conf_txt.right(wl_conf_txt.length()-9);
            }
        }

        if ( wg_local_port == "" )
        {
            wg_conf_host_data = "127.0.0.1";
        }

        if ( wg_local_port == "" )
        {
            wg_local_port = "1976";
        }

        ui->label_tcp_port->setText(wg_local_port);
    }

    //

    QString wl_myip = "";
    QString wl_myip_full = "";

    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const QHostAddress &address: QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
        {
            wl_myip = address.toString();
            wl_myip_full = wl_myip;
            int wl_last_index = wl_myip.lastIndexOf(".");
            wl_myip = wl_myip.left(wl_last_index);
            wg_ip_svq = wl_myip + ".";
            wl_last_index = wl_myip_full.lastIndexOf(".");
            wg_last_octet = wl_myip_full.right((wl_myip_full.length() - wl_last_index)-1);
            wg_full_ip = wl_myip_full;

            ui->label_ip_listen->setText(wg_full_ip);
        }
    }

    listen_server();

    guardian_timer = new QTimer(this);
    connect(guardian_timer, SIGNAL(timeout()), this, SLOT(listen_server()));
    guardian_timer->start(60000);

    wg_username = qgetenv("USER");
    if ( wg_username.length() < 1 )
    {
        wg_username = qgetenv("USERNAME");
    }

    // *** RECUPERATION D'UN FICHIER CONTENANT LES IP CIBLES ***
    manager = new QNetworkAccessManager(this);
    QUrl myurl = QUrl(wg_conf_host_data);
    reply = manager->get(QNetworkRequest(myurl));
    connect(reply, SIGNAL(finished()),
                this, SLOT(httpDownloadFinished()));
}

void MainWindow::httpDownloadFinished()
{
    if( reply->error() == QNetworkReply::NoError )
    {
        QString wl_file_content = reply->readAll();
        wl_file_content = wl_file_content.remove(QRegExp("[\\n\\t\\r]"));
        wg_file_liste = wl_file_content.split(";");

        QStringList wl_data;

        foreach (QString wl_pc, wg_file_liste)
        {
            wl_data = wl_pc.split("|");
            if ( wl_data.count() == 2 && wg_full_ip != wl_data[0] )
            {
                ui->My_Combo_IP->addItem(wl_data[1] + " -> " + wl_data[0]);
            }
        }
        ui->label_arp_ok->setText(QString::number(ui->My_Combo_IP->count()) + " IP clientes récupérées");
    }
    else
    {
        ui->label_arp_ok->setText("Aucune donnée. Balayage plage IP.");
    }

    ui->txttosend_free->setDisabled(true);
    ui->txttosend->setDisabled(true);
    ui->My_Combo_IP->setDisabled(true);
    ui->Mybt_free->setDisabled(true);
    ui->Mybt_admin->setDisabled(true);
}

// *** PARTIE SERVEUR ***

void MainWindow::listen_server()
{
    if(!mytcpsrv->isListening())
    {
        QIcon iconoff(":/new/prefix1/Images/icon_off.png");
        trayIcon->setIcon(iconoff);

        ui->Icon_server_status->setPixmap(QPixmap(":/new/prefix1/Images/srv_nok.png"));
        ui->Label_server_status->setText("Le serveur n'a pas pu démarrer");

        restart_server();
    }
    else
    {
        QIcon iconoff(":/new/prefix1/Images/icon.png");
        trayIcon->setIcon(iconoff);

        ui->Icon_server_status->setPixmap(QPixmap(":/new/prefix1/Images/srv_ok.png"));
        ui->Label_server_status->setText("Le serveur est correctement démarré");
    }
}

void MainWindow::restart_server()
{
    if(mytcpsrv->isListening())
    {
        mytcpsrv->close();
    }

    if(!mytcpsrv->listen(QHostAddress::Any, wg_local_port.toInt()))
    {
        QIcon iconoff(":/new/prefix1/Images/icon_off.png");
        trayIcon->setIcon(iconoff);

        ui->Icon_server_status->setPixmap(QPixmap(":/new/prefix1/Images/srv_nok.png"));
        ui->Label_server_status->setText("Le serveur n'a pas pu démarrer");
    }
    else
    {
        QIcon iconoff(":/new/prefix1/Images/icon.png");
        trayIcon->setIcon(iconoff);

        ui->Icon_server_status->setPixmap(QPixmap(":/new/prefix1/Images/srv_ok.png"));
        ui->Label_server_status->setText("Le serveur est correctement démarré");
    }
}

void MainWindow::new_srv_connect()
{
    ui->My_logs->appendHtml("CONNECTION ENTERING ....");
    server_socket = mytcpsrv->nextPendingConnection();
    connect(server_socket, SIGNAL(connected()),this, SLOT(srv_connected()));
    connect(server_socket, SIGNAL(disconnected()),this, SLOT(srv_disconnected()));
    connect(server_socket, SIGNAL(bytesWritten(qint64)),this, SLOT(srv_bytesWritten(qint64)));
    connect(server_socket, SIGNAL(readyRead()),this, SLOT(srv_readyRead()));

    server_timer->start();
}

void MainWindow::srv_connected()
{
    qDebug() << "SOCKET SERVEUR CONNECTED";
}

void MainWindow::srv_disconnected()
{
}

void MainWindow::srv_readyRead()
{
    QByteArray ba = server_socket->readAll();
    QByteArray mystring = "TOK";
    qDebug() << ba;
    QString wl_txt_display = QString::fromLatin1(ba);
    server_socket->write(mystring);
    server_socket->flush();

    if ( wl_txt_display.left(7) == "[SOUN1]" )
    {
        sound_alarm->setLoops(4);
        sound_alarm->play();

        wl_txt_display = wl_txt_display.right(wl_txt_display.length()-7);
    }
    else
    {
        if ( wl_txt_display.left(7) == "[SOUN2]" )
        {
            sound_pinpon->setLoops(8);
            sound_pinpon->play();

            wl_txt_display = wl_txt_display.right(wl_txt_display.length()-7);
        }
    }

    m_alert_dialog->setalerttxt(wl_txt_display);
    m_alert_dialog->setWindowFlags ( Qt::FramelessWindowHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowStaysOnTopHint );
    m_alert_dialog->setWindowState( m_alert_dialog->windowState() | Qt::WindowFullScreen);
    m_alert_dialog->show();
    m_alert_dialog->raise();
}

void MainWindow::srv_bytesWritten(qint64)
{
    server_timer->stop();
    server_socket->close();
    server_socket->disconnectFromHost();
}

void MainWindow::timeout_server()
{
    server_socket->close();
    server_socket->disconnectFromHost();
}

// *** PARTIE CLIENT ***

void MainWindow::connected()
{
    QByteArray textTemp;
    QByteArray textComp = "\n\nMessage de : " + wg_username.toLatin1();

    if ( wg_type_message == 0 )
    {
        textTemp = ui->txttosend->toPlainText().toLatin1();
    }
    else
    {
        QString wl_sound = "";

        if ( wg_type_message == 1 )
        {
            if ( ui->Check_notify_sound->isChecked() )
            {
                wl_sound = "[SOUN1]";
            }

            QString wl_txt = wl_sound + "Intrusion dans le bâtiment.";
            textTemp = wl_txt.toLatin1();
        }
        else
        {
            if ( wg_type_message == 3 )
            {
                textTemp = ui->txttosend_free->toPlainText().toLatin1();
            }
            else
            {
                if ( ui->Check_notify_sound->isChecked() )
                {
                    wl_sound = "[SOUN2]";
                }

                QString wl_txt = wl_sound + "Incendie dans le bâtiment.\nVeuillez évacuer dans le calme.";
                textTemp = wl_txt.toLatin1();
            }
        }
    }

    textTemp = textTemp + textComp;

    client_socket->write(textTemp);
    client_socket->flush();
    client_timer->start();
}

void MainWindow::timeout_client()
{
    client_socket->disconnectFromHost();
}

void MainWindow::disconnected()
{
    client_timer->stop();
    loop_ip();
}

void MainWindow::readyRead()
{
    client_timer->stop();
    if ( client_socket->readAll() == "TOK")
    {
        ui->My_logs->appendHtml("ECHANGE COMPLET");
        wg_success++;
    }
    else
    {
        ui->My_logs->appendHtml("ECHANGE INCOMPLET");
    }
    client_socket->disconnectFromHost();
}
void MainWindow::bytesWritten(qint64)
{
}

// *** PARTIE BOUCLE DU TRAITEMENT D'ENVOIS DES MESSAGES ***

void MainWindow::loop_message()
{
    if ( wg_octet < wg_loop_max )
    {
        ui->Pb_send->setValue(wg_octet);
        QString wl_ip_txt = "";
        if ( wg_file_liste.count() > 0 && wg_type_message != 0 )
        {
            QStringList wl_data = wg_file_liste[wg_octet-1].split("|");
            if ( wl_data.count() == 2 && wg_full_ip != wl_data[0] )
            {
                wl_ip_txt = wl_data[0];
                client_socket->connectToHost(wl_data[0],mytcpsrv->serverPort(), QIODevice::ReadWrite);
            }
            else
            {
                loop_ip();
                return;
            }
        }
        else
        {
            if ( wg_type_message != 0 )
            {
                if ( wg_octet != wg_last_octet.toInt() )
                {
                    wl_ip_txt = wg_ip_svq + QString::number(wg_octet);
                    client_socket->connectToHost(wg_ip_svq + QString::number(wg_octet),mytcpsrv->serverPort(), QIODevice::ReadWrite);
                }
                else
                {
                    loop_ip();
                    return;
                }
            }
            else
            {
                QString wl_current_target = ui->My_Combo_IP->currentText();
                QStringList wl_data = wl_current_target.split(" -> ");
                wl_ip_txt = wl_data[1];

                client_socket->connectToHost(wg_ip_svq + QString::number(wg_octet),mytcpsrv->serverPort(), QIODevice::ReadWrite);
            }
        }

        if(client_socket->waitForConnected(40))
        {
            ui->My_logs->appendHtml("UP : " + wl_ip_txt);
        }
        else
        {
            ui->My_logs->appendHtml("DOWN : " + wl_ip_txt);
            loop_ip();
        }
    }
}

void MainWindow::loop_ip()
{
    wg_octet++;
    if ( wg_octet < wg_loop_max )
    {
        QTimer::singleShot(4, this, SLOT(loop_message()));
    }
    else
    {
        ui->My_logs->appendHtml("-------------");

        if ( wg_success > 0 )
        {
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setText("Message transmis.");
            msgBox.exec();
        }
        else
        {
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setText("Personne n'a reçu votre message.");
            msgBox.exec();
        }

        ui->tabWidget->setDisabled(false);
    }
}

// *** PARTIE GESTION UI ***

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::display_interface()
{
    this->show();
}

void MainWindow::on_Bt_close_ui_clicked()
{
    this->hide();
}

// *** BOUTON DE LANCEMENT DES ALERTES ***

void MainWindow::on_Mybt_intrusion_clicked()
{
    ui->tabWidget->setDisabled(true);
    wg_octet = 1;
    wg_type_message = 1;
    if ( wg_file_liste.count() > 0 )
    {
        wg_loop_max = wg_file_liste.count();
    }
    else
    {
        wg_loop_max = 255;
    }
    wg_success = 0;
    ui->Pb_send->setMinimum(0);
    ui->Pb_send->setMaximum(wg_loop_max-1);
    loop_message();
}

void MainWindow::on_Mybt_incendie_clicked()
{
    ui->tabWidget->setDisabled(true);
    wg_octet = 1;
    wg_type_message = 2;
    if ( wg_file_liste.count() > 0 )
    {
        wg_loop_max = wg_file_liste.count();
    }
    else
    {
        wg_loop_max = 255;
    }
    wg_success = 0;
    ui->Pb_send->setMinimum(0);
    ui->Pb_send->setMaximum(wg_loop_max-1);
    loop_message();
}

void MainWindow::on_Mybt_admin_clicked()
{
    ui->tabWidget->setDisabled(true);
    wg_type_message = 0;
    wg_success = 0;

    QString wl_current_target = ui->My_Combo_IP->currentText();
    QStringList wl_data = wl_current_target.split(" -> ");
    QString wl_myip = wl_data[1];

    int wl_last_index = wl_myip.lastIndexOf(".");
    wl_myip = wl_myip.right(wl_myip.length()-(wl_last_index+1));
    wg_octet = wl_myip.toInt();
    wg_loop_max = wl_myip.toInt()+1;

    ui->Pb_send->setMinimum(wg_octet-1);
    ui->Pb_send->setMaximum(wg_octet);
    loop_message();
}

void MainWindow::on_Mybt_free_clicked()
{
    ui->tabWidget->setDisabled(true);
    wg_octet = 1;
    wg_type_message = 3;
    if ( wg_file_liste.count() > 0 )
    {
        wg_loop_max = wg_file_liste.count();
    }
    else
    {
        wg_loop_max = 255;
    }
    wg_success = 0;
    ui->Pb_send->setMinimum(0);
    ui->Pb_send->setMaximum(wg_loop_max-1);
    loop_message();
}

void MainWindow::on_Bt_admin_clicked()
{
    bool ok;
    QString wl_passwd = QInputDialog::getText(this, "ACCES FONCTIONS ADMIN", "Code administrateur :", QLineEdit::Normal, "", &ok);
    if ( ok && !wl_passwd.isEmpty() && wl_passwd == wg_admin_pwd )
    {
        ui->txttosend_free->setEnabled(true);
        ui->Mybt_free->setEnabled(true);
        if ( ui->My_Combo_IP->count() > 0 )
        {
            ui->txttosend->setEnabled(true);
            ui->My_Combo_IP->setEnabled(true);
            ui->Mybt_admin->setEnabled(true);
        }
        ui->Bt_admin->setDisabled(true);
    }
}
