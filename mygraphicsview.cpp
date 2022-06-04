#include "mygraphicsview.h"
//#include <QMouseEvent>

#include <QDebug>

MyGraphicsView::MyGraphicsView(QWidget *parent) : QGraphicsView(parent)
{
//    setMouseTracking(false);
}

MyGraphicsView::~MyGraphicsView()
{

}


//void MyGraphicsView::mouseMoveEvent(QMouseEvent* event)
//{
//    QPoint pos = event->pos();
//    qDebug() << "From GraphicsView: " << pos.x() << "," << pos.y();
//    emit SendScenePos(pos);
//}
