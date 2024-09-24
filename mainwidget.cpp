#include "mainwidget.h"
#include "./ui_mainwidget.h"

#include <QDebug>
#include <QtConcurrent>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);

    stream = new IpCamera();
    connect(stream, &StreamInterface::frameReady, this, &MainWidget::updateView, Qt::QueuedConnection);

    connect(ui->pbStop, &QPushButton::clicked, [this] () {
        if (stream->stopFlag.test_and_set()) {
            stream->stopFlag.clear();
            stream->startStreamingRoutine();
            ui->pbStop->setText("Stop");
        }
        else {
            stream->stop();
            sender->pause();
            ui->pbStop->setText("Stream");
        }
    });

    connect(ui->cbSource, &QComboBox::currentIndexChanged, [this] (int index) {
        switch (index) {
        case 0:
            stream->stop();
            stream->deleteSelf();
            ui->pbStop->setText("Stream");

            stream = new IpCamera();
            connect(stream, &StreamInterface::frameReady, this, &MainWidget::updateView, Qt::QueuedConnection);
            break;
        case 1:
            stream->stop();
            stream->deleteSelf();
            ui->pbStop->setText("Stream");

            stream = new UsbCamera();
            connect(stream, &StreamInterface::frameReady, this, &MainWidget::updateView);
            connect(this, &MainWidget::destroyed, stream, &StreamInterface::deleteLater);
            break;
        case 2:
            stream->stop();
            stream->deleteSelf();
            ui->pbStop->setText("Stream");

            stream = new UsbCamera12Bit();

            stream->pa.setStartingTime();
            stream->pa.makeNote("started analysis");
            connect(stream, &StreamInterface::frameReady, this, &MainWidget::updateView);
            connect(this, &MainWidget::destroyed, stream, &StreamInterface::deleteLater);
            break;
        default:
            break;
        }
    });


    connect(ui->pbConnect, &QPushButton::clicked, [this] () {
        qDebug() << "Trying to connect to" << ui->leIp->text() << "port" << ui->sbPort->value();
        sender->establishConnection(QHostAddress {ui->leIp->text()}, ui->sbPort->value());
    });

    stream->pa.setStartingTime();
    stream->stopFlag.test_and_set();

    sender = new TcpSender(this);
    reader = new TcpReader(this);
    connect(sender, &TcpSender::receivedPacket, reader, &TcpReader::decodePacket);
    connect(reader, &TcpReader::frameDecoded, this, &MainWidget::updateViewFromTcpReader);

    connect(ui->pbSetUpReader, &QPushButton::clicked, [this] (int index) {
        reader->setup({ui->sbReaderWidth->value(), ui->sbReaderHeight->value()});
    });

}

MainWidget::~MainWidget()
{
    stream->stopFlag.test_and_set();
    stream->commitSudokuFlag.test_and_set();
    delete ui;
}

void MainWidget::updateView()
{
    stream->mutex.lock();
    QPixmap pm = QPixmap::fromImage(stream->lastFrame);
    sender->sendFrame(stream->lastFrame);
    stream->mutex.unlock();

    ui->lbVideoArea->setPixmap(pm);

    stream->pa.makeNote("finished frame processing");

    stream->pa.calcAvg();

}

void MainWidget::updateViewFromTcpReader(const QImage &frame)
{
    QPixmap pm = QPixmap::fromImage(frame);
    ui->lbVideoArea->setPixmap(pm);
}




