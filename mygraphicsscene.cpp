#include "mygraphicsscene.h"
#include <QGraphicsSceneMouseEvent>

#include <QDebug>

MyGraphicsScene::MyGraphicsScene(QObject *parent) : QGraphicsScene(parent), mMouseLButtonPressedAndTracking_bool(false), mImageArea_rect(QRect()), mRoi_rectitem(nullptr)
{

}

MyGraphicsScene::~MyGraphicsScene()
{

}

void MyGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    QPointF pF = mouseEvent->scenePos();
    QPoint p = QPoint(static_cast<int>(pF.x()), static_cast<int>(pF.y())); //QPointF::toPoint() is not adequate as it returns the rounded coordinates
    emit SendCursorScenePos(p);

    if ((mouseEvent->buttons() & Qt::LeftButton) && mMouseLButtonPressedAndTracking_bool)
    {
        if(mImageArea_rect.contains(p))
        {
            QRect r(mRoiPoint1_point, p);
            mRoi_rectitem->setRect(r);
        }
    }
}

void MyGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (mouseEvent->button() == Qt::LeftButton)
    {
        QPointF pF = mouseEvent->scenePos();
        QPoint p = QPoint(static_cast<int>(pF.x()), static_cast<int>(pF.y())); //QPointF::toPoint() is not adequate as it returns the rounded coordinates
//        qDebug() << "mousePressEvent reached.";
//        qDebug() << "mImageArea_rect: " << mImageArea_rect;
//        qDebug() << "p: " << p;
//        qDebug() << "mImageArea_rect.contains(p): " << mImageArea_rect.contains(p);
        if(mImageArea_rect.contains(p))
        {
            qDebug() << "mousePressed->items().count(): " << items().count();
            mRoiPoint1_point = p;
            if(mRoi_rectitem)
            {
                removeItem(mRoi_rectitem);
                //mRoi_rectitem = nullptr;
                delete mRoi_rectitem;
                mRoi_rectitem = nullptr;
            }

            mRoi_rectitem = new QGraphicsRectItem;
            QPen pen(Qt::green, 3, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin);
            mRoi_rectitem->setPen(pen);
            addItem(mRoi_rectitem);

            mMouseLButtonPressedAndTracking_bool = true;
        }
    }
}

void MyGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (mouseEvent->button() == Qt::LeftButton && mMouseLButtonPressedAndTracking_bool)
    {
        QPointF pF = mouseEvent->scenePos();
        QPoint p = QPoint(static_cast<int>(pF.x()), static_cast<int>(pF.y())); //QPointF::toPoint() is not adequate as it returns the rounded coordinates
        qDebug() << "mouseReleaseEvent reached.";
        if(mImageArea_rect.contains(p))
        {
            QRect r(mRoiPoint1_point, p);
            mRoi_rectitem->setRect(r);
            emit SendRoi(r.normalized());
        }
        else
        {
            removeItem(mRoi_rectitem);
            //mRoi_rectitem = nullptr;
            delete mRoi_rectitem;
            mRoi_rectitem = nullptr;
        }
        mMouseLButtonPressedAndTracking_bool = false;
    }
}

void MyGraphicsScene::CleanMyMembers()
{
    if(mRoi_rectitem)
    {
        qDebug() << "CleanMyMembers() -> just before removeItem()";
        removeItem(mRoi_rectitem);
        //mRoi_rectitem = nullptr;
        qDebug() << "CleanMyMembers() -> just before delete";
        delete mRoi_rectitem;
        mRoi_rectitem = nullptr;
    }
}

void MyGraphicsScene::ResetImageArea(QRect rect)
{
    mImageArea_rect = rect;
    setSceneRect(rect);
}

void MyGraphicsScene::AddRoi(QRect roi)
{
    qDebug() << "AddRoi() is reached.";
    if(mRoi_rectitem)
    {
        qDebug() << "AddRoi() -> just before removeItem()";
        removeItem(mRoi_rectitem);
        //mRoi_rectitem = nullptr;
        qDebug() << "AddRoi() -> just before delete";
        delete mRoi_rectitem;
        mRoi_rectitem = nullptr;
    }

    mRoi_rectitem = new QGraphicsRectItem;
    QPen pen(Qt::green, 3, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin);
    mRoi_rectitem->setPen(pen);
    addItem(mRoi_rectitem);

    mRoi_rectitem->setRect(roi);
    emit SendRoi(roi.normalized());
}

