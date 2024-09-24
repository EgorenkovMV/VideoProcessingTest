#include "usbcamera12bit.h"
#include <opencv2/imgproc.hpp>

void UsbCamera12Bit::resendFrame(const QVideoFrame &frame)
{
    static int verboseFramesCount {0};
    verboseFramesCount++;

    pa.makeNote("new frame! Restarting timer...");
    pa.setStartingTime();


    QVideoFrame frameReadable {frame};
    frameReadable.map(QVideoFrame::MapMode::ReadOnly);
    pa.makeNote("buffer not ready");


    const uchar *bytesRead = frameReadable.bits(0);
    uchar *buffer = new uchar[frameReadable.width() * frameReadable.height() * 2] ();
    std::memcpy(buffer, bytesRead, frameReadable.width() * frameReadable.height() * 2);
    pa.makeNote("buffer ready");


    // Copy the data into an OpenCV Mat structure
    cv::Mat mat16uc1_bayer(frameReadable.height(), frameReadable.width(), CV_16UC1, reinterpret_cast<void *>(buffer));
    pa.makeNote("mat16uc1_bayer constructed");


    // Decode the Bayer data to RGB but keep using 16 bits per channel
    cv::Mat mat16uc3_rgb(frameReadable.height(), frameReadable.width(), CV_16UC3);
    cv::cvtColor(mat16uc1_bayer, mat16uc3_rgb, cv::COLOR_BayerGR2RGB);
    pa.makeNote("demosaiced");


    // Convert the 16-bit (12-bit) per channel RGB image to 8-bit per channel
    cv::Mat mat8uc3_rgb(frameReadable.height(), frameReadable.width(), CV_8UC3);
    mat16uc3_rgb.convertTo(mat8uc3_rgb, CV_8UC3, 1.0/16.);
    pa.makeNote("converted 16 bit -> 8 bit");

    QImage imgIn = QImage((uchar*) mat8uc3_rgb.data, mat8uc3_rgb.cols, mat8uc3_rgb.rows, mat8uc3_rgb.step, QImage::Format_RGB888);
    pa.makeNote("copied Mat.data to imgIn");

    delete [] buffer;
    pa.makeNote("deleted buffer");

    frameReadable.unmap();
    pa.makeNote("unmapped frameReadable");

    lastFrame = imgIn;
    pa.makeNote("copied QImage");

    lastFrame.bits();
    pa.makeNote("deepcopied QImage");

    emit frameReady();
    pa.makeNote("returning from UsbCamera12Bit::resendFrame");
}



