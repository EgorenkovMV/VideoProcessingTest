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
    void deleteSelf() override;

    AVFrame	*lastNativeFrame;   ////// debug

private:
    void startWrapper();
    int decodePacket(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame, AVCodecParameters *pCodecParameters);
    void hibernate();

};

#endif // IPCAMERA_H
