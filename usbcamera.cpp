#include "usbcamera.h"
#include <QDebug>
#include <QCameraDevice>
#include <QMediaDevices>
#include <QThread>
#include <QByteArray>

UsbCamera::UsbCamera(QObject *parent)
    : StreamInterface{parent}
{
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &cameraDevice : cameras) {
        qDebug() << "Found camera:" << cameraDevice.description();
        for (const QCameraFormat &format : cameraDevice.videoFormats()) {
            qDebug() << "Avaliable format: pixFormat:" << format.pixelFormat() <<
                        "| res:" << format.resolution() <<
                        "| fps:" << format.minFrameRate() << "-" << format.maxFrameRate();
        }
    }

    for (const QCameraDevice &cameraDevice : cameras) {
        qDebug() << "Taking first system camera:" << cameraDevice.description();
        camera = new QCamera(cameraDevice);
        break;
    }

    stopFlag.test_and_set();
    commitSudokuFlag.clear();

    if (camera == nullptr) {
        return;
    }

    captureSession.setCamera(camera);
    videoSink = new QVideoSink;
    captureSession.setVideoOutput(videoSink);
}

UsbCamera::~UsbCamera()
{
    delete camera;
    delete videoSink;
}

void UsbCamera::startStreamingRoutine()
{
    connect(videoSink, &QVideoSink::videoFrameChanged, this, &UsbCamera::resendFrame);
}

void UsbCamera::stop()
{
    stopFlag.test_and_set();
    if (videoSink != nullptr) {
        disconnect(videoSink, &QVideoSink::videoFrameChanged, this, &UsbCamera::resendFrame);
    }
}

void UsbCamera::deleteSelf()
{
    delete this;
}

void UsbCamera::resendFrame(const QVideoFrame &frame)
{
    pa.makeNote("new frame! Restarting timer...");
    pa.setStartingTime();

    QImage im = frame.toImage();
    im = im.convertToFormat(QImage::Format_RGB888);
    lastFrame = im;

    emit frameReady();
}

