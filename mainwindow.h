#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QReadWriteLock>
//#include <QGraphicsScene>
#include "mygraphicsscene.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

typedef QPair<qreal, qreal> StatisticsPair;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionOpenAnImage_triggered();
    void on_actionOpenADirectory_triggered();
    void on_actionExit_triggered();
    void ReceiveCursorScenePos(const QPoint scenePos);
    void ReceiveRoi(const QRect roi);
    void receiveMessage(QString msg);

    void on_pushButtonBatchCalculation_clicked();
    //void on_lineEditTopLeftX_textEdited(const QString &arg1);
    //void on_lineEditTopLeftY_textEdited(const QString &arg1);
    //void on_lineEditBottomRightX_textEdited(const QString &arg1);
    //void on_lineEditBottomRightY_textEdited(const QString &arg1);

//    void on_lineEditGrayLevelAdjustmentMin_editingFinished();
//    void on_lineEditGrayLevelAdjustmentMax_editingFinished();
    void on_lineEditTopLeftX_editingFinished();
    void on_lineEditTopLeftY_editingFinished();
    void on_lineEditBottomRightX_editingFinished();
    void on_lineEditBottomRightY_editingFinished();

    void on_spinBoxGrayLevelAdjustmentMin_editingFinished();
    void on_spinBoxGrayLevelAdjustmentMin_valueChanged(int arg1);
    void on_spinBoxGrayLevelAdjustmentMax_editingFinished();
    void on_spinBoxGrayLevelAdjustmentMax_valueChanged(int arg1);
    void on_doubleSpinBoxRotationAngle_editingFinished();
    void on_doubleSpinBoxRotationAngle_valueChanged(double arg1);
    void on_pushButtonFixRotation_clicked();

    void on_actionZoomIn_triggered();

    void on_actionZoomOut_triggered();

    void on_actionAbout_triggered();

private:
    Ui::MainWindow *ui;
    QImage mImage;
    QImage* mImageReduced;
    MyGraphicsScene *mImageScene;
    QGraphicsPixmapItem *mImageScenePixmapItem;
    QDir* mDirectory;
    qreal mScale;
    qreal mRotationAngle;
    QReadWriteLock *mLock;

    quint16 RetrieveGrayScaleValue(int x, int y, QImage &img);
    StatisticsPair GrayScaleValueStatistics(const QRect roi, QImage &img);
    void CheckRoiInput();
    void AdjustGrayScale();
    void DoBatchCalculation(QFileInfoList imgFiles_fileInfoList, QRect roi, qreal fullLiquid, qreal fullSolid);
//    void MedianFilt(QImage &img);
//    void MedianFilt2(QImage &imgRef, QImage &img);
signals:
    void message(QString msg);
};
#endif // MAINWINDOW_H
