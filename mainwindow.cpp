#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QtMath>
#include <QDoubleValidator>
#include <QMessageBox>
#include <QFuture>
#include <QtConcurrent>

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), mImage(nullptr), mImageReduced(nullptr), mImageScene(nullptr), mImageScenePixmapItem(nullptr), mDirectory(nullptr), mScale(0.25), mRotationAngle(0.0)
{
    ui->setupUi(this);
    setWindowTitle("Assessing Liquid Fraction");
    mImageScene = new MyGraphicsScene(this);
    ui->imageView->setScene(mImageScene);
    ui->lineEditTopLeftX->setValidator(new QDoubleValidator(0, 9999, 0, this));
    ui->lineEditTopLeftY->setValidator(new QDoubleValidator(0, 9999, 0, this));
    ui->lineEditBottomRightX->setValidator(new QDoubleValidator(0, 9999, 0, this));
    ui->lineEditBottomRightY->setValidator(new QDoubleValidator(0, 9999, 0, this));
    ui->lineEditFullLiquidFractionGrayLevel->setValidator(new QDoubleValidator(0, 65535, 2, this));
    ui->lineEditFullSolidFractionGrayLevel->setValidator(new QDoubleValidator(0, 65535, 2, this));
    mDirectory = new QDir(QDir::currentPath());
//    ui->lineEditGrayLevelAdjustmentMin->setValidator(new QDoubleValidator(0, 65535, 0, this));
//    ui->lineEditGrayLevelAdjustmentMax->setValidator(new QDoubleValidator(0, 65535, 0, this));
//    ui->lineEditRotationAngle->setValidator(new QDoubleValidator(-999, 999, 1, this));
    //connect(ui->imageView, SIGNAL(SendScenePos(QPoint)), this, SLOT(ReceiveScenePos(QPoint)));

    // below were working connects before June 1st
//    connect(mImageScene, SIGNAL(SendCursorScenePos(QPoint)), this, SLOT(ReceiveCursorScenePos(QPoint)));
//    connect(mImageScene, SIGNAL(SendRoi(QRect)), this, SLOT(ReceiveRoi(QRect)));

    connect(mImageScene, &MyGraphicsScene::SendCursorScenePos, this, &MainWindow::ReceiveCursorScenePos);
    connect(mImageScene, &MyGraphicsScene::SendRoi, this, &MainWindow::ReceiveRoi);
    connect(this, &MainWindow::message, this, &MainWindow::receiveMessage);
    mLock = new QReadWriteLock;
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_actionOpenAnImage_triggered()
{
//    QString path = QDir::currentPath();
//    path.truncate(path.lastIndexOf("/"));
//    path.append("/test_images/normalized");

    QString filePath = QFileDialog::getOpenFileName(this, "Open an Image File", mDirectory->absolutePath(), "Image Files (*.tif *.tiff *.png)");
    if (!filePath.isEmpty())
    {
        if(!mImage.load(filePath))
        {
            ui->textEditResults->append(QString("<b>Error!</b> Cannot open image file: \"%1\".").arg(filePath));
            return;
        }

        mRotationAngle = ui->doubleSpinBoxRotationAngle->value();
        if(!(mRotationAngle == -720. || mRotationAngle == -360. || mRotationAngle == 0. || mRotationAngle == 360. ||  mRotationAngle == 720.))
        {
            QTransform tr;
            tr.rotate(mRotationAngle);
            mImage = mImage.transformed(tr, Qt::SmoothTransformation);
        }

        mImageScene->CleanMyMembers();
        mImageScene->clear();                   //
        ui->imageView->resetTransform();        //
        ui->imageView->scale(mScale, mScale);   //

        if(mImageReduced != nullptr)
        {
            delete mImageReduced;
            mImageReduced = nullptr;
        }
        //mImageReduced = new QImage(mImage.rect().width(), mImage.rect().height(), QImage::Format_Grayscale8);
        mImageReduced = new QImage(mImage.size(), QImage::Format_Grayscale8);
        mImageScenePixmapItem = mImageScene->addPixmap(QPixmap::fromImage(*mImageReduced));
        AdjustGrayScale();
        //QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(*mImageReduced));
        //mImageScene->addPixmap(item);
        //mImageScene->addItem(item);
        mImageScene->ResetImageArea(mImageReduced->rect());
        mImageScene->update();                              //?
        //ui->imageView->setSceneRect(mImageReduced->rect());
        QString fileName = QFileInfo(filePath).fileName();
        ui->labelFileNameValue->setText(fileName);
        QString directory(filePath);
        directory.truncate(directory.lastIndexOf("/"));
        mDirectory->setPath(directory);
        //mDirectory = new QDir(directory);
        QFontMetrics metrics(ui->labelFileNameValue->font());
        QString elidedFilePath = metrics.elidedText(directory, Qt::ElideLeft, ui->labelFileNameValue->width());
        ui->labelDirectoryValue->setText(elidedFilePath);

        CheckRoiInput();
    }
}

void MainWindow::on_actionOpenADirectory_triggered()
{
    QString directory = QFileDialog::getExistingDirectory(this, "Open Directory", mDirectory->absolutePath(), QFileDialog::ShowDirsOnly);
    if (!directory.isEmpty())
        mDirectory->setPath(directory);
    QFontMetrics metrics(ui->labelFileNameValue->font());
    QString elidedFilePath = metrics.elidedText(directory, Qt::ElideLeft, ui->labelFileNameValue->width());
    ui->labelDirectoryValue->setText(elidedFilePath);
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::ReceiveCursorScenePos(const QPoint scenePos)
{
    int x = scenePos.x();
    int y = scenePos.y();
    QString cursorPositionInfo;

    if(!mImage.isNull())
    {
        if(mImage.valid(x, y))
        {
            //int byte = mImage.depth()/8;
            quint16 value = RetrieveGrayScaleValue(x, y, mImage);
            cursorPositionInfo = (QString::asprintf("(%d, %d): %d", x, y, value));
            //quint16 *value = (quint16*) (mImage.bits() + mImage.bytesPerLine()*y + byte*x);
            //cursorPositionInfo = (QString::asprintf("(%d, %d): %d", x, y, *value));
        }
    }
    statusBar()->showMessage(cursorPositionInfo);
}

void MainWindow::ReceiveRoi(const QRect roi)
{
    ui->lineEditTopLeftX->setText(QString::number(roi.left()));
    ui->lineEditTopLeftY->setText(QString::number(roi.top()));
    ui->lineEditBottomRightX->setText(QString::number(roi.right()));
    ui->lineEditBottomRightY->setText(QString::number(roi.bottom()));
    ui->lineEditWidth->setText(QString::number(roi.width()));
    ui->lineEditHeight->setText(QString::number(roi.height()));
    StatisticsPair s = GrayScaleValueStatistics(roi, mImage);
    ui->lineEditMean->setText(QString::asprintf("%7.1f", s.first));
    ui->lineEditStd->setText(QString::asprintf("%7.1f", s.second));
}

void MainWindow::receiveMessage(QString msg)
{
    ui->textEditResults->append(msg);
}

quint16 MainWindow::RetrieveGrayScaleValue(int x, int y, QImage &img)
{
    //int byte = mImage.depth()/8;
    int byte = img.depth()/8;
    //quint16 *value = (quint16*) (mImage.bits() + mImage.bytesPerLine()*y + byte*x);
    quint16 *value = (quint16*) (img.bits() + img.bytesPerLine()*y + byte*x);
    return *value;
}

StatisticsPair MainWindow::GrayScaleValueStatistics(const QRect roi, QImage &img)
{
    int left = roi.left();
    int top = roi.top();
    int right = roi.right();
    int bottom = roi.bottom();
    qreal pixelNumber = static_cast<qreal>(roi.width() * roi.height());

    qreal grayLevelValueSum = 0.;
    for(int j=top; j<=bottom; j++)
        for(int i=left; i<=right; i++)
            grayLevelValueSum += RetrieveGrayScaleValue(i, j, img);
    qreal grayLevelValueMean = grayLevelValueSum/pixelNumber;

    qreal grayLevelDeviationSquaredSum = 0.;
    for(int j=top; j<=bottom; j++)
        for(int i=left; i<=right; i++)
            grayLevelDeviationSquaredSum += qPow(((static_cast<qreal>(RetrieveGrayScaleValue(i, j, img)))-grayLevelValueMean), 2);
            //grayLevelDeviationSquaredSum += ((static_cast<qreal>(RetrieveGrayScaleValue(i, j, mImage)))-grayLevelValueMean)*((static_cast<qreal>(RetrieveGrayScaleValue(i, j, mImage)))-grayLevelValueMean);
    qreal grayLevelValueStd = qSqrt(grayLevelDeviationSquaredSum/pixelNumber);

    StatisticsPair s(grayLevelValueMean, grayLevelValueStd);

    return s;
}

void MainWindow::CheckRoiInput()
{
    if(mImage.isNull())
        return;

    if(!(ui->lineEditTopLeftX->text().isEmpty() || ui->lineEditTopLeftY->text().isEmpty() || ui->lineEditBottomRightX->text().isEmpty() || ui->lineEditBottomRightY->text().isEmpty()))
    {
        int topLeftX = ui->lineEditTopLeftX->text().toInt();
        int topLeftY = ui->lineEditTopLeftY->text().toInt();
        int bottomRightX = ui->lineEditBottomRightX->text().toInt();
        int bottomRightY = ui->lineEditBottomRightY->text().toInt();
        QPoint topLeft(topLeftX, topLeftY);
        QPoint bottomRight(bottomRightX, bottomRightY);
        QRect roi(topLeft, bottomRight);
        if(!roi.isValid())
        {
            ui->textEditResults->append("ROI rectangle setting is invalid.");
            return;
        }
        if(mImage.rect().contains(roi))
            mImageScene->AddRoi(roi);
        else
        {
            ui->lineEditMean->clear();
            ui->lineEditStd->clear();
        }
    }
}

void MainWindow::AdjustGrayScale()
{
    if(mImage.isNull())
        return;

    qreal min = static_cast<qreal>(ui->spinBoxGrayLevelAdjustmentMin->value());
    qreal max = static_cast<qreal>(ui->spinBoxGrayLevelAdjustmentMax->value());
    if(max > 65535)
    {
        ui->textEditResults->append("<b>Warning</b> Keep Max <= 65535 in GrayLevelAdjustment.");
        return;
    }
    if(min < 0)
    {
        ui->textEditResults->append("<b>Warning</b> Keep min >= 0 in GrayLevelAdjustment.");
        return;
    }
    if(!(min < max))
    {
        ui->textEditResults->append("<b>Warning</b> Keep min < Max in GrayLevelAdjustment.");
        return;
    }
    QRect fov(mImageReduced->rect());
    int left = fov.left();
    int top = fov.top();
    int right = fov.right();
    int bottom = fov.bottom();

    int byte = mImageReduced->depth()/8;
    uchar *bits = mImageReduced->bits();
    int bytesPerLine = mImageReduced->bytesPerLine();
    for(int j=top; j<=bottom; j++)
        for(int i=left; i<=right; i++)
        {
            quint8 *value = (quint8*) (bits + bytesPerLine * j + byte * i);
            qreal reduced = 255./(max - min)*(static_cast<qreal>(RetrieveGrayScaleValue(i, j, mImage)) - min);
            if(reduced > 255.)
                reduced = 255.;
            if(reduced < 0.)
                reduced = 0.;
            *value = static_cast<quint8>(reduced);
        }
    mImageScenePixmapItem->setPixmap(QPixmap::fromImage(*mImageReduced));
}

void MainWindow::DoBatchCalculation(QFileInfoList imgFiles_fileInfoList, QRect roi, qreal fullLiquid, qreal fullSolid)
{
//    ui->textEditResults->append("---------------------------------------");
    emit message(QString("---------------------------------------"));
//    ui->textEditResults->append(QString("Folder name: %1").arg(mDirectory->absolutePath()));
    emit message(QString("Folder name: %1").arg(mDirectory->absolutePath()));
//    ui->textEditResults->append(QString("Rotation angle: %1").arg(mRotationAngle));
    emit message(QString("Rotation angle: %1").arg(mRotationAngle));
//    ui->textEditResults->append(QString("ROI rectangle: (%1, %2) to (%3, %4)").arg(roi.left()).arg(roi.top()).arg(roi.right()).arg(roi.bottom()));
    emit message(QString("ROI rectangle: (%1, %2) to (%3, %4)").arg(roi.left()).arg(roi.top()).arg(roi.right()).arg(roi.bottom()));
//    ui->textEditResults->append(QString("Full-liquid gray level value: %1").arg(fullLiquid, 7, 'f', 1));
    emit message(QString("Full-liquid gray level value: %1").arg(fullLiquid, 7, 'f', 1));
//    ui->textEditResults->append(QString("Full-solid gray level value: %1").arg(fullSolid, 7, 'f', 1));
    emit message(QString("Full-solid gray level value: %1").arg(fullSolid, 7, 'f', 1));
//    ui->textEditResults->append(QString("%1 image files are found in the directory:").arg(imgFiles_fileInfoList.count()));
    emit message(QString("%1 image files are found in the directory:").arg(imgFiles_fileInfoList.count()));

    QStringList fileNameFilter_stringList = {"data_??.txt"};
    QStringList dataFiles_stringList = mDirectory->entryList(fileNameFilter_stringList, QDir::Files, QDir::Name|QDir::IgnoreCase);
    int lastDataFileNumber;
    if(dataFiles_stringList.isEmpty())
        lastDataFileNumber = 0;
    else
    {
        QString lastDataFile_string = dataFiles_stringList.last();
        lastDataFile_string.truncate(lastDataFile_string.lastIndexOf(".txt"));
        lastDataFile_string.remove(lastDataFile_string.indexOf("data_"), 5);
        lastDataFileNumber = lastDataFile_string.toInt();
    }

    QStringList directoryNameFilter_stringList = {"data_*"};
    QStringList dataDirectories_stringList = mDirectory->entryList(directoryNameFilter_stringList, QDir::Dirs, QDir::Name|QDir::IgnoreCase);
    int lastDataDirectoryNumber;
    if(dataDirectories_stringList.isEmpty())
        lastDataDirectoryNumber = 0;
    else
    {
        QString lastDataDirectory_string = dataDirectories_stringList.last();
        lastDataDirectory_string.remove(lastDataDirectory_string.indexOf("data_"), 5);
        lastDataDirectoryNumber = lastDataDirectory_string.toInt();
    }

    int newDataNumber = 1 + ((lastDataFileNumber >= lastDataDirectoryNumber)?lastDataFileNumber:lastDataDirectoryNumber);
    QString newDataFileName = QString::asprintf("data_%02d.txt", newDataNumber);
    QString newDataFilePath = mDirectory->absolutePath() + "/" + newDataFileName;
    QString newDataDirectoryName = QString::asprintf("data_%02d", newDataNumber);
    QString newDataDirectoryPath = mDirectory->absolutePath() + "/" + newDataDirectoryName;

    QFile dataFile(newDataFilePath);
    if(!dataFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
//        ui->textEditResults->append(QString("<b>Error!</b> Data file (\"%1\") is not created.").arg(newDataFileName));
        emit message(QString("<b>Error!</b> Data file (\"%1\") is not created.").arg(newDataFileName));
        return;
    }
    QTextStream stream(&dataFile);
    stream << QString("Folder name: %1").arg(mDirectory->absolutePath()) << Qt::endl;
    stream << QString("Rotation angle: %1").arg(mRotationAngle) << Qt::endl;
    stream << QString("ROI rectangle: (%1, %2) to (%3, %4)").arg(roi.left()).arg(roi.top()).arg(roi.right()).arg(roi.bottom()) << Qt::endl;
    stream << QString("Full-liquid gray level value: %1").arg(fullLiquid, 7, 'f', 1) << Qt::endl;
    stream << QString("Full-solid gray level value: %1").arg(fullSolid, 7, 'f', 1) << Qt::endl;
    stream << QString("Index | \tFilename | \tLiquid fraction mean | \t std") << Qt::endl;

    if(!mDirectory->mkdir(newDataDirectoryName))
    {
//        ui->textEditResults->append(QString("<b>Error!</b> Data directory (\"%1\") is not created.").arg(newDataDirectoryName));
        emit message(QString("<b>Error!</b> Data directory (\"%1\") is not created.").arg(newDataDirectoryName));
        return;
    }

    int index = 0;
    int processed = 0;
    for(auto &&imgFile:imgFiles_fileInfoList)
    {
        index++;
        QString imgFilePath = imgFile.absoluteFilePath();
        QImage img;
        mLock->lockForRead();
        bool result = img.load(imgFilePath);
        mLock->unlock();
        if(!result)
        {
//            ui->textEditResults->append(QString("<b>Info</b> \"%1\" is not open: ignored.").arg(imgFilePath));
            emit message(QString("<b>Info</b> \"%1\" is not open: ignored.").arg(imgFilePath));
            break;
        }
        if(!(mRotationAngle == -720. || mRotationAngle == -360. || mRotationAngle == 0. || mRotationAngle == 360. ||  mRotationAngle == 720.))
        {
            QTransform tr;
            tr.rotate(mRotationAngle);
            img = img.transformed(tr, Qt::SmoothTransformation);
        }
        if(!(img.rect().contains(roi)))
        {
//            ui->textEditResults->append(QString("<b>Info</b> \"%1\" does not contain the ROI: ignored.").arg(imgFilePath));
            emit message(QString("<b>Info</b> \"%1\" does not contain the ROI: ignored.").arg(imgFilePath));
            break;
        }
        processed++;
        //MedianFilt(img);
        int left = roi.left();
        int top = roi.top();
        int right = roi.right();
        int bottom = roi.bottom();
        int width = roi.width();
        int height = roi.height();
        QImage liquidFractionDataImg(width, height, QImage::Format_Grayscale16);
        int byte = liquidFractionDataImg.depth()/8;
        uchar *bits = liquidFractionDataImg.bits();
        int bytesPerLine = liquidFractionDataImg.bytesPerLine();
        for(int j=top; j<=bottom; j++)
        {
            int y = j - top;
            for(int i=left; i<=right; i++)
            {
                int x = i - left;
                quint16 *value = (quint16*) (bits + bytesPerLine * y + byte * x);
                qreal liquidFraction = (static_cast<qreal>(RetrieveGrayScaleValue(i, j, img)) - fullSolid)/(fullLiquid - fullSolid)*10000. + 20000.;
                if(liquidFraction > 65535.)
                    liquidFraction = 65535.;
                if(liquidFraction < 0.)
                    liquidFraction = 0.;
                *value = static_cast<quint16>(liquidFraction);
                //*value = static_cast<quint16>((static_cast<qreal>(RetrieveGrayScaleValue(i, j, img)) - fullSolid)/(fullLiquid - fullSolid)*10000.);
            }
        }
        StatisticsPair s = GrayScaleValueStatistics(liquidFractionDataImg.rect(), liquidFractionDataImg);
        qreal mean = (s.first - 20000.)/100.;
        qreal std = s.second/100.;
//        ui->textEditResults->append(QString("%1 \"%2\" Mean: %3, Std: %4").arg(index).arg(imgFile.fileName()).arg(mean, 7, 'f', 4).arg(std, 7, 'f', 4));
        emit message(QString("%1 \"%2\" Mean: %3, Std: %4").arg(index).arg(imgFile.fileName()).arg(mean, 7, 'f', 4).arg(std, 7, 'f', 4));
        mLock->lockForWrite();
        stream << QString("%1\t%2\t%3\t%4").arg(index).arg(imgFile.fileName()).arg(mean, 7, 'f', 4).arg(std, 7, 'f', 4) << Qt::endl;
        mLock->unlock();
        //QString liquidFractionDataImgFilePath(newDataDirectoryPath + "/" + imgFile.fileName());
        QString liquidFractionDataImgFilePath = newDataDirectoryPath + QDir::separator() + imgFile.fileName();
        mLock->lockForWrite();
        liquidFractionDataImg.save(liquidFractionDataImgFilePath);
        mLock->unlock();
    }
//    ui->textEditResults->append(QString("Total %1 image files are processed. Data is stored in \"%2\", and data images are stored in \"%3\" folder.").arg(processed).arg(newDataFileName).arg(newDataDirectoryName));
    emit message(QString("Total %1 image files are processed. Data is stored in \"%2\", and data images are stored in \"%3\" folder.").arg(processed).arg(newDataFileName).arg(newDataDirectoryName));
    dataFile.close();
}

void MainWindow::on_pushButtonBatchCalculation_clicked()
{
    if(mDirectory == nullptr)
    {
        ui->textEditResults->append("<b>Error!</b> Directory is not specified.");
        return;
    }

    QString roiTopLeftXInput = ui->lineEditTopLeftX->text();
    QString roiTopLeftYInput = ui->lineEditTopLeftY->text();
    QString roiWidthInput = ui->lineEditWidth->text();
    QString roiHeightInput = ui->lineEditHeight->text();
    if(roiTopLeftXInput.isEmpty() || roiTopLeftYInput.isEmpty() || roiWidthInput.isEmpty() || roiHeightInput.isEmpty())
    {
        ui->textEditResults->append("<b>Error!</b> ROI input is not complete.");
        return;
    }
    int roiTopLeftX = roiTopLeftXInput.toInt();
    int roiTopLeftY = roiTopLeftYInput.toInt();
    int roiWidth = roiWidthInput.toInt();
    int roiHeight = roiHeightInput.toInt();
    QRect roi(roiTopLeftX, roiTopLeftY, roiWidth, roiHeight);

    QString fullLiquidTextInput = ui->lineEditFullLiquidFractionGrayLevel->text();
    QString fullSolidTextInput = ui->lineEditFullSolidFractionGrayLevel->text();
    if(fullLiquidTextInput.isEmpty() || fullSolidTextInput.isEmpty())
    {
        ui->textEditResults->append("<b>Error!</b> Reference input is missing.");
        return;
    }
    qreal fullLiquid = fullLiquidTextInput.toDouble();
    qreal fullSolid = fullSolidTextInput.toDouble();
    if(fullLiquid <= fullSolid)
    {
        ui->textEditResults->append("<b>Error!</b> Full-LiquidFractionGrayLevel should be larger than Full-SolidFractionGrayLevel.");
        return;
    }

    QStringList imgNameFilter_stringList = {"*.tif", "*.tiff", "*.png"};
    QFileInfoList imgFiles_fileInfoList = mDirectory->entryInfoList(imgNameFilter_stringList, QDir::Files, QDir::Name|QDir::IgnoreCase);
    if(imgFiles_fileInfoList.isEmpty())
    {
        ui->textEditResults->append("<b>Warning</b> No image files (\".tif\", \".tiff\", or \".png\") are detected in the directory.");
        ui->textEditResults->append("The process is terminated.");
        return;
    }


    QFuture<void> future = QtConcurrent::run(this, &MainWindow::DoBatchCalculation, imgFiles_fileInfoList, roi, fullLiquid, fullSolid);
}

//void MainWindow::on_lineEditGrayLevelAdjustmentMin_editingFinished()
//{
//    AdjustGrayScale();
//}

//void MainWindow::on_lineEditGrayLevelAdjustmentMax_editingFinished()
//{
//    AdjustGrayScale();
//}

void MainWindow::on_lineEditTopLeftX_editingFinished()
{
    CheckRoiInput();
}

void MainWindow::on_lineEditTopLeftY_editingFinished()
{
    CheckRoiInput();
}

void MainWindow::on_lineEditBottomRightX_editingFinished()
{
    CheckRoiInput();
}

void MainWindow::on_lineEditBottomRightY_editingFinished()
{
    CheckRoiInput();
}

void MainWindow::on_spinBoxGrayLevelAdjustmentMin_editingFinished()
{
    AdjustGrayScale();
}

void MainWindow::on_spinBoxGrayLevelAdjustmentMin_valueChanged(int arg1)
{
    qreal min = static_cast<qreal>(ui->spinBoxGrayLevelAdjustmentMin->value());
    qreal max = static_cast<qreal>(ui->spinBoxGrayLevelAdjustmentMax->value());
    if(!(min < max))
        return;
    AdjustGrayScale();
}

void MainWindow::on_spinBoxGrayLevelAdjustmentMax_editingFinished()
{
    AdjustGrayScale();
}

void MainWindow::on_spinBoxGrayLevelAdjustmentMax_valueChanged(int arg1)
{
    qreal min = static_cast<qreal>(ui->spinBoxGrayLevelAdjustmentMin->value());
    qreal max = static_cast<qreal>(ui->spinBoxGrayLevelAdjustmentMax->value());
    if(!(min < max))
        return;
    AdjustGrayScale();
}

void MainWindow::on_doubleSpinBoxRotationAngle_editingFinished()
{
    //mRotationAngle = ui->doubleSpinBoxRotationAngle->value();
    qreal angle = ui->doubleSpinBoxRotationAngle->value();
    ui->imageView->rotate(angle - mRotationAngle);
    mRotationAngle = angle;
}

void MainWindow::on_doubleSpinBoxRotationAngle_valueChanged(double arg1)
{
    ui->imageView->rotate(arg1 - mRotationAngle);
    mRotationAngle = arg1;
}

void MainWindow::on_pushButtonFixRotation_clicked()
{
    if(mImage.isNull())
        return;
    if((mRotationAngle == -720. || mRotationAngle == -360. || mRotationAngle == 0. || mRotationAngle == 360. ||  mRotationAngle == 720.))
        return;

    QTransform tr;
    tr.rotate(mRotationAngle);
    mImage = mImage.transformed(tr, Qt::SmoothTransformation);

    mImageScene->CleanMyMembers();
    mImageScene->clear();                   //
    ui->imageView->resetTransform();        //
    ui->imageView->scale(mScale, mScale);   //

    if(mImageReduced != nullptr)
    {
        delete mImageReduced;
        mImageReduced = nullptr;
    }
    mImageReduced = new QImage(mImage.size(), QImage::Format_Grayscale8);
    mImageScenePixmapItem = mImageScene->addPixmap(QPixmap::fromImage(*mImageReduced));
    AdjustGrayScale();
    mImageScene->ResetImageArea(mImageReduced->rect());
    mImageScene->update();

    ui->lineEditMean->clear();
    ui->lineEditStd->clear();

    ui->pushButtonFixRotation->setEnabled(false);
    ui->textEditResults->append("<b>Info</b> The \"FixRotation!\" pushbutton is disabled. If you want to change RotationAngle next time, you have to turn off and re-run the program. Sorry for the inconveniences.");
}

void MainWindow::on_actionZoomIn_triggered()
{
    mScale = mScale * 2.;
    qDebug() << "mScale: " << mScale;
    ui->imageView->scale(2., 2.);
    //ui->imageView->update();
}


void MainWindow::on_actionZoomOut_triggered()
{
    mScale = mScale * 0.5;
    qDebug() << "mScale: " << mScale;
    ui->imageView->scale(0.5, 0.5);
    //ui->imageView->update();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About this program",
            QString("<p>The program <b>Assessing Liquid Fraction</b> was developed by "
                "Dr. Jae-Hong Lim at Beamline 6C Bio Medical Imaging of the Pohang Light Source-II, "
                "Pohang Accelerator Laboratory. </p>"
                "<p>For any suggestions and trouble report, please email to limjh@postech.ac.kr. </p>"
));
}
