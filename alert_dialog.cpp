#include "alert_dialog.h"
#include "ui_alert_dialog.h"
#include "frame_th_trans.h"
#include <QDateTime>

alert_dialog::alert_dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::alert_dialog)
{
    ui->setupUi(this);

    logo_warning = new frame_th_trans(this);
    this->setLayout(ui->Alert_layout);
}

alert_dialog::~alert_dialog()
{
    delete ui;
}

void alert_dialog::setalerttxt(QString wl_txt)
{
    ui->alert_dialog_txt->setText(wl_txt + "\n" + QDateTime::currentDateTime().toString("dddd dd MMMM yyyy hh:mm:ss"));
    if ( logo_warning->is_visible == false )
    {
        logo_warning->is_visible = true;
        logo_warning->render_prev();
    }
}

void alert_dialog::on_pushButton_clicked()
{
    logo_warning->is_visible = false;
    this->hide();
}

void alert_dialog::resizeEvent(QResizeEvent *)
{
    logo_warning->setGeometry(this->width()-310,10,300,300);
}
