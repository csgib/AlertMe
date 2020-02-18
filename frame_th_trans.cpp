#include <QtGui>
#include "frame_th_trans.h"
frame_th_trans::frame_th_trans ( QWidget * parent )
    : QFrame(parent)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setAttribute(Qt::WA_TranslucentBackground, true);
    dstImage.load(":/new/prefix1/Images/alarma.png");

    x = 60;
    y = 60;
    w = 190;
    h = 190;
    o = 1;

    render_prev();
}

void frame_th_trans::render_prev()
{
    if ( x < 0 )
    {
        x = 60;
        y = 60;
        w = 190;
        h = 190;
        o = 1.0;
    }
    else
    {
        o = o - 0.017;
        x = x - 1;
        y = y - 1;
        w = w + 2;
        h = h + 2;
    }

    QTimer::singleShot((60-x)/2, this, SLOT(render_prev()));
    this->repaint();
}


// ********************************
// *** DESSIN DU FOND DU WIDGET ***
// ********************************

void frame_th_trans::paintEvent(QPaintEvent *)
{
    QPainter p;
    p.begin(this);
    p.setRenderHint(QPainter::Antialiasing,true);
    p.setOpacity(1-o);
    p.drawImage(QRect(60,60,190,190),dstImage);
    p.setOpacity(o);
    p.drawImage(QRect(x,y,w,h),dstImage);
    p.end();
}

