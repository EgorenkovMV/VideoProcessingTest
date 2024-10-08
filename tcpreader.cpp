#include "tcpreader.h"
#include <QDebug>


TcpReader::TcpReader(QObject *parent)
    : QObject{parent}
{
    frameRGB = av_frame_alloc();
    frameYUV = av_frame_alloc();
}

TcpReader::~TcpReader()
{
    pause();
    av_frame_free(&frameRGB);
    av_frame_free(&frameYUV);
    if (imgConvertCtx) {
        sws_freeContext(imgConvertCtx);
    }
}

void TcpReader::setup(const QSize &size)
{
    if (isSetUp) {
        pause();
    }

    codec = avcodec_find_decoder(AVCodecID::AV_CODEC_ID_H264);
    if (!codec) {
        qDebug() << "codec not found";
        exit(1);
    }

    context = avcodec_alloc_context3(codec);

    qDebug() << "\n\tTcpReader::setup";

    context->width = size.width();
    context->height = size.height();
    context->time_base = (AVRational){1, 25};
    context->framerate = (AVRational){25, 1};
    context->gop_size = 10;
    context->max_b_frames = 1;
    context->pix_fmt = AVPixelFormat::AV_PIX_FMT_YUV420P;
    context->bit_rate = 400'000;


    if (avcodec_open2(context, codec, nullptr) < 0) {
        qDebug() << "could not open codec";
        exit(1);
    }

    frameCounter = 0;


    frameRGB->format = AVPixelFormat::AV_PIX_FMT_RGB24;
    frameRGB->width = context->width;
    frameRGB->height = context->height;
    if (av_frame_get_buffer(frameRGB, 0) < 0) {
        qDebug() << "could not get buffer for frameYUV";
        exit(1);
    }


    frameYUV->format = AVPixelFormat::AV_PIX_FMT_YUV420P;
    frameYUV->width = context->width;
    frameYUV->height = context->height;


    imgConvertCtx = sws_getCachedContext(imgConvertCtx,
                                         context->width, context->height,
                                         AVPixelFormat::AV_PIX_FMT_YUV420P,
                                         context->width, context->height,
                                         AVPixelFormat::AV_PIX_FMT_RGB24,
                                         SWS_BICUBIC, nullptr, nullptr, nullptr);

    isSetUp = true;

}

void TcpReader::decodePacket(AVPacket *packet)
{
    if (not isSetUp) {
        qDebug() << "TCPReader is not set up" << frameCounter;
        return;
    }

    qDebug() << "\n\tStarted decoding frame" << frameCounter;

    av_frame_unref(frameYUV);
    frameYUV->pts = frameCounter;
    frameCounter++;


    qDebug() << "decoding frame: avcodec_send_packet" << avcodec_send_packet(context, packet);


    int err = 0;
    while (err >= 0) {
        err = avcodec_receive_frame(context, frameYUV);
        if (not err) {
            sws_scale(imgConvertCtx, frameYUV->data, frameYUV->linesize, 0,
                      context->height, frameRGB->data, frameRGB->linesize);

            QImage frame {frameRGB->data[0],
                          frameRGB->width,
                          frameRGB->height,
                          frameRGB->linesize[0],
                          QImage::Format_RGB888};

            emit frameDecoded(frame);
        }
        qDebug() << "decoding frame: avcodec_receive_frame" << err;
    }
}

void TcpReader::pause()
{
    avcodec_close(context);
    av_free(context);
}






