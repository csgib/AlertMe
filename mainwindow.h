#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include <QTimer>
#include <QSound>
#include <QSystemTrayIcon>
#include <QNetworkConfiguration>
#include "alert_dialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    alert_dialog *m_alert_dialog;

    QTcpServer *mytcpsrv;
    QTcpSocket *client_socket;
    QTcpSocket *server_socket;

    QTimer *client_timer;
    QTimer *server_timer;
    QTimer *guardian_timer;

    int wg_octet;
    int wg_loop_max;
    int wg_type_message;
    int wg_success;
    QString wg_last_octet;
    QString wg_username;
    QString wg_full_ip;
    QString wg_ip_svq;

    QString wg_conf_host_data;
    QString wg_local_port;

    QSound *sound_alarm;
    QSound *sound_pinpon;

    QStringList wg_file_liste;

public slots:
    // *** INTERFACE DECLARATION ***
    void display_interface();
    void on_Bt_close_ui_clicked();

private slots:
    // *** NETWORK DECLARATIONS ***
    void connected();
    void disconnected();
    void readyRead();
    void bytesWritten(qint64);

    void new_srv_connect();

    void srv_connected();
    void srv_disconnected();
    void srv_readyRead();
    void srv_bytesWritten(qint64);

    void timeout_client();
    void timeout_server();

    // *** BOUTONS LANCEMENT ALERTES ***
    void on_Mybt_intrusion_clicked();
    void on_Mybt_incendie_clicked();
    void on_Mybt_admin_clicked();

    // *** GUARDIAN ***
    void restart_server();
    void listen_server();

    // *** TRAITEMENT ***
    void loop_message();
    void loop_ip();

    // *** FICHIERS CONF ***
    void httpDownloadFinished();

    void on_Mybt_free_clicked();

    void on_Bt_admin_clicked();

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *trayIcon;

    QNetworkAccessManager *manager;
    QNetworkReply *reply;

    QString wg_admin_pwd;
};

#endif // MAINWINDOW_H
