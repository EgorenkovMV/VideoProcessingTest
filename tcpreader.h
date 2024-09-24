#ifndef TCPREADER_H
#define TCPREADER_H

#include <QObject>
#include <QImage>


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

class TcpReader : public QObject
{
    Q_OBJECT
public:
    explicit TcpReader(QObject *parent = nullptr);
    ~TcpReader();

    void setup(const QSize &size);


signals:
    void frameDecoded(const QImage &frame);

public slots:
    void decodePacket(AVPacket *packet);
    void pause();

private:
    int frameCounter          = 0;
    AVCodec *codec            = nullptr;
    AVCodecContext *context   = nullptr;
    AVFrame *frameRGB         = nullptr;
    AVFrame *frameYUV         = nullptr;
    SwsContext *imgConvertCtx = nullptr;

    bool isSetUp = false;

};

#endif // TCPREADER_H
