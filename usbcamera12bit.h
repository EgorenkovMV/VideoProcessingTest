#ifndef USBCAMERA12BIT_H
#define USBCAMERA12BIT_H

#include <utility>
#include "usbcamera.h"

class UsbCamera12Bit : public UsbCamera
{
    enum class Mosaic
    {
        G1 = 0,
        B,
        R,
        G2,
    };
public:
    explicit UsbCamera12Bit(QObject *parent = nullptr) : UsbCamera(parent) {}

private slots:
    void resendFrame(const QVideoFrame &frame);

};

#endif // USBCAMERA12BIT_H
