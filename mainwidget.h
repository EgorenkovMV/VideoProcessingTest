#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QGraphicsScene>
#include <QMutex>
#include <QTimer>
#include <atomic>
#include "ipcamera.h"
#include "usbcamera.h"
#include "usbcamera12bit.h"
#include "tcpsender.h"
#include "tcpreader.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWidget; }
QT_END_NAMESPACE

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

private:
    Ui::MainWidget *ui;

    StreamInterface *stream;

    TcpSender *sender;
    TcpReader *reader;

private slots:
    void updateView();
    void updateViewFromTcpReader(const QImage &frame);


};
#endif // MAINWIDGET_H
