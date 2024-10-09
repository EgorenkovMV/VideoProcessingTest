#ifndef IPCAMERA_H
#define IPCAMERA_H

#include "streaminterface.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

class IpCamera : public StreamInterface
{
public:
    explicit IpCamera(QObject *parent = nullptr);
    ~IpCamera();

    void startStreamingRoutine() override;
    void stop() override;
    QImage getLastFrame() override;

    QMutex mutex;

private:
    std::atomic_flag routineFinishedFlag;

    void startWrapper();
    int decodePacket(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame, AVCodecParameters *pCodecParameters);

};

#endif // IPCAMERA_H
