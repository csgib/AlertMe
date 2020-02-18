#ifndef frame_th_trans_H
#define frame_th_trans_H
#include <QFrame>

class frame_th_trans : public QFrame
{
	Q_OBJECT
	public:
                frame_th_trans(QWidget * parent);
                int x,y,w,h;
                qreal o;
                QImage dstImage;
	public:
	private:
	signals:
	public slots:
                void render_prev();
	protected:
                void paintEvent(QPaintEvent *);
};
#endif
