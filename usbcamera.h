#ifndef USBCAMERA_H
#define USBCAMERA_H


#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include "streaminterface.h"

class UsbCamera : public StreamInterface
{
public:
    explicit UsbCamera(QObject *parent = nullptr);
    ~UsbCamera();

    void startStreamingRoutine() override;
    void stop() override;
    QImage getLastFrame() override;


private:
    QCamera *camera = nullptr;
    QMediaCaptureSession captureSession;
    QVideoSink *videoSink = nullptr;

private slots:
    virtual void resendFrame(const QVideoFrame &frame);

};


#endif // USBCAMERA_H
