#ifndef MYGRAPHICSVIEW_H
#define MYGRAPHICSVIEW_H

#include <QGraphicsView>

class MyGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    MyGraphicsView(QWidget *parent = nullptr);
    ~MyGraphicsView();
//signals:
//    void SendScenePos(const QPoint &pos);
//protected:
//    void mouseMoveEvent(QMouseEvent* event) override;
};

#endif // MYGRAPHICSVIEW_H
