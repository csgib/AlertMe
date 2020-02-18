#ifndef ALERT_DIALOG_H
#define ALERT_DIALOG_H

#include <QDialog>
#include "frame_th_trans.h"

namespace Ui {
class alert_dialog;
}

class alert_dialog : public QDialog
{
    Q_OBJECT

public:
    explicit alert_dialog(QWidget *parent = nullptr);
    ~alert_dialog();
    frame_th_trans *logo_warning;

public slots:
    void setalerttxt(QString);

private slots:
    void on_pushButton_clicked();
    void resizeEvent(QResizeEvent *);

private:
    Ui::alert_dialog *ui;

};

#endif // ALERT_DIALOG_H
