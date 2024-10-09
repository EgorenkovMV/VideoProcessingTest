#include "mainwidget.h"
#include "./ui_mainwidget.h"

#include <QDebug>
#include <QtConcurrent>

namespace Config {
// TODO: make config pretty
constexpr bool    useNNForwarder  = true;
constexpr float   debugFps        = 25.f;
const     QString modelDirPath    = "../pretrained_models/mobilenet2_coco/";

}

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
        delete stream;
        ui->pbStop->setText("Stream");

        switch (index) {
        case 0:
            stream = new IpCamera();
            stream->pa.setStartingTime();
            stream->pa.makeNote("started analysis with IpCamera");

            connect(stream, &StreamInterface::frameReady, this, &MainWidget::updateView, Qt::QueuedConnection);
            break;
        case 1:
            stream = new UsbCamera();
            stream->pa.setStartingTime();
            stream->pa.makeNote("started analysis with UsbCamera");

            connect(stream, &StreamInterface::frameReady, this, &MainWidget::updateView);
            break;
        case 2:
            stream = new UsbCamera12Bit();
            stream->pa.setStartingTime();
            stream->pa.makeNote("started analysis with UsbCamera12Bit");

            connect(stream, &StreamInterface::frameReady, this, &MainWidget::updateView);
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

    connect(ui->sbPortReceiver, &QSpinBox::valueChanged, sender, &TcpSender::listenToPort);
    sender->listenToPort(ui->sbPortReceiver->value());

    connect(ui->pbSetUpReader, &QPushButton::clicked, [this] (int index) {
        reader->setup({ui->sbReaderWidth->value(), ui->sbReaderHeight->value()});
    });

    forwarder = new NNForwarder(this);
    forwarder->fps = Config::debugFps;
    forwarder->dirPath = Config::modelDirPath;

}

MainWidget::~MainWidget()
{
    delete stream;
    delete sender;
    delete reader;
    delete forwarder;
    delete ui;
}

void MainWidget::updateView()
{
//    qDebug() << "Thread id in MainWidget::updateView" << QThread::currentThreadId();

    QImage nnResult;
    if (Config::useNNForwarder) {
        nnResult = forwarder->forward(stream->getLastFrame());
    }
    else {
        nnResult = stream->getLastFrame();
    }

    sender->sendFrame(nnResult);

    QPixmap pm = QPixmap::fromImage(nnResult);
    ui->lbVideoArea->setPixmap(pm);

    stream->pa.makeNote("finished frame processing");

    stream->pa.calcAvg();

}

void MainWidget::updateViewFromTcpReader(const QImage &frame)
{
    QPixmap pm = QPixmap::fromImage(frame);
    ui->lbVideoArea->setPixmap(pm);
}




