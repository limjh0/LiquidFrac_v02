#ifndef MYGRAPHICSSCENE_H
#define MYGRAPHICSSCENE_H

#include <QGraphicsScene>
#include <QGraphicsRectItem>

class MyGraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    MyGraphicsScene(QObject *parent = nullptr);
    ~MyGraphicsScene();
    void ResetImageArea(QRect rect);
    void AddRoi(QRect roi);
    void CleanMyMembers();
protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
signals:
    void SendCursorScenePos(const QPoint scenePos);
    void SendRoi(const QRect rect);
private:
    bool mMouseLButtonPressedAndTracking_bool;
    QRect mImageArea_rect;
    QPoint mRoiPoint1_point;
    QGraphicsRectItem* mRoi_rectitem;
};

#endif // MYGRAPHICSSCENE_H
