#include "ipcamera.h"
#include <QDebug>
#include <QtConcurrent>

IpCamera::IpCamera(QObject *parent)
    : StreamInterface{parent}
{
    stopFlag.test_and_set();
    commitSudokuFlag.clear();
}

IpCamera::~IpCamera()
{

}

void IpCamera::startStreamingRoutine()
{

    std::ignore = QtConcurrent::run(&IpCamera::startWrapper, this);

}

void IpCamera::stop()
{
    stopFlag.test_and_set();
}

void IpCamera::deleteSelf()
{
    commitSudokuFlag.test_and_set();
}


void IpCamera::startWrapper()
{
    pa.makeNote("new frame! Restarting timer...");
    pa.setStartingTime();

    AVFormatContext *pFormatContext = avformat_alloc_context();
    if (!pFormatContext) {
        qDebug() << "ERROR could not allocate memory for Format Context";
        checkSudoku();
        return;
    }


    if (avformat_open_input(&pFormatContext, "rtsp://admin:admin@192.168.1.10:554", nullptr, nullptr) != 0) {
        qDebug() << "ERROR could not open the file";
        checkSudoku();
        return;
    }

    qDebug().nospace() << "format " << pFormatContext->iformat->name << ", duration " << pFormatContext->duration << " us, bit_rate " << pFormatContext->bit_rate;


    if (avformat_find_stream_info(pFormatContext,  nullptr) < 0) {
        qDebug() << "ERROR could not get the stream info";
        checkSudoku();
        return;
    }

    AVCodec *pCodec = nullptr;

    AVCodecParameters *pCodecParameters = nullptr;
    int video_stream_index = -1;


    for (int i = 0; i < pFormatContext->nb_streams; i++) {
        AVCodecParameters *pLocalCodecParameters =  NULL;
        pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
        qDebug() << "AVStream->time_base before open coded" << pFormatContext->streams[i]->time_base.num << "/" << pFormatContext->streams[i]->time_base.den;
        qDebug() << "AVStream->r_frame_rate before open coded" << pFormatContext->streams[i]->r_frame_rate.num << "/" << pFormatContext->streams[i]->r_frame_rate.den;
        qDebug() << "AVStream->start_time" << pFormatContext->streams[i]->start_time;
        qDebug() << "AVStream->duration" << pFormatContext->streams[i]->duration;

        qDebug() << "finding the proper decoder (CODEC)";

        AVCodec *pLocalCodec = NULL;

        // finds the registered decoder for a codec ID
        pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

        if (pLocalCodec == NULL) {
            qDebug() << "ERROR unsupported codec!";
            continue;
        }

        // when the stream is a video we store its index, codec parameters and codec
        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (video_stream_index == -1) {
                video_stream_index = i;
                pCodec = pLocalCodec;
                pCodecParameters = pLocalCodecParameters;
            }

            qDebug() << "Video Codec: resolution" << pLocalCodecParameters->width << "x" << pLocalCodecParameters->height;
        } else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            qDebug() << "Audio Codec:"  << pLocalCodecParameters->channels << " channels, sample rate" << pLocalCodecParameters->sample_rate;
        }

        // print its name, id and bitrate
        qDebug() << "\tCodec" << pLocalCodec->name << "ID" << pLocalCodec->id << "bit_rate" << pLocalCodecParameters->bit_rate;
    }


    if (video_stream_index == -1) {
        qDebug() << "File does not contain a video stream!";
        checkSudoku();
        return;
    }

    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext)
    {
        qDebug() << "failed to allocated memory for AVCodecContext";
        checkSudoku();
        return;
    }

    if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0)
    {
        qDebug() << "failed to copy codec params to codec context";
        checkSudoku();
        return;
    }

    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0)
    {
        qDebug() << "failed to open codec through avcodec_open2";
        checkSudoku();
        return;
    }

    AVFrame *pFrame = av_frame_alloc();
    if (!pFrame)
    {
        qDebug() << "failed to allocate memory for AVFrame";
        checkSudoku();
        return;
    }

    AVPacket *pPacket = av_packet_alloc();
    if (!pPacket)
    {
        qDebug() << "failed to allocate memory for AVPacket";
        checkSudoku();
        return;
    }

    int response = 0;

    // fill the Packet with data from the Stream
    while (av_read_frame(pFormatContext, pPacket) >= 0)
    {
        if (pPacket->stream_index == video_stream_index) {
            pa.makeNote("new frame! Restarting timer...");
            pa.setStartingTime();
            response = decodePacket(pPacket, pCodecContext, pFrame, pCodecParameters);
            if (response < 0) break;

            if (stopFlag.test_and_set()) {
                break;
            }
            else {
                stopFlag.clear();
            }
        }
        av_packet_unref(pPacket);
    }

    avformat_close_input(&pFormatContext);
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecContext);

    hibernate();
    return;
}


int IpCamera::decodePacket(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame, AVCodecParameters *pCodecParameters)
{
    // Supply raw packet data as input to a decoder
    int response = avcodec_send_packet(pCodecContext, pPacket);

    if (response < 0) {
        qDebug() << "Error while sending a packet to the decoder:" << response;
        return response;
    }

    AVFrame	*pFrameRGB = av_frame_alloc();
    pFrameRGB->format = AV_PIX_FMT_RGB24;
    pFrameRGB->width = pCodecParameters->width;
    pFrameRGB->height = pCodecParameters->height;
    int err = av_frame_get_buffer(pFrameRGB, 0);
    if (err != 0) {
        qDebug() << "Error while allocating new buffer:" << response;
        return response;
    }
    SwsContext *imgConvertCtx = nullptr;


    while (response >= 0)
    {
        // Return decoded output data (into a frame) from a decoder
        response = avcodec_receive_frame(pCodecContext, pFrame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            qDebug() << "Error while receiving a frame from the decoder:" << response;

            av_frame_unref(pFrameRGB);
            if (imgConvertCtx) sws_freeContext(imgConvertCtx);
            return response;
        }


        if (response >= 0) {
//            qDebug().nospace() <<
//            "Frame " << pCodecContext->frame_number <<
//            " (type=" << av_get_picture_type_char(pFrame->pict_type) <<
//            ", size=" << pFrame->pkt_size <<
//            " bytes, format=" << pFrame->format <<
//            ") pts " << pFrame->pts <<
//            " key_frame " << pFrame->key_frame <<
//            " [DTS " << pFrame->coded_picture_number << "]";

            imgConvertCtx = sws_getCachedContext(imgConvertCtx,
                                                 pCodecParameters->width, pCodecParameters->height,
                                                 static_cast<AVPixelFormat>(pCodecParameters->format),
                                                 pFrameRGB->width, pFrameRGB->height,
                                                 AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr);
            if (imgConvertCtx == nullptr) {
                qDebug() << "imgConvertCtx is nullptr(((";
            }

            //conversion frame to frameRGB
            sws_scale(imgConvertCtx, pFrame->data, pFrame->linesize, 0, pCodecContext->height, pFrameRGB->data, pFrameRGB->linesize);

            QImage im {pFrameRGB->data[0],
                       pFrameRGB->width,
                       pFrameRGB->height,
                       pFrameRGB->linesize[0],
                       QImage::Format_RGB888};

            mutex.lock();
            lastFrame = im;
            lastFrame.bits();
            mutex.unlock();
            emit frameReady();
        }
    }
    av_frame_free(&pFrameRGB);
    if (imgConvertCtx) sws_freeContext(imgConvertCtx);

    return 0;
}


void IpCamera::hibernate()
{
    forever {

        QThread::msleep(200);
        if (not stopFlag.test_and_set()) {
            stopFlag.clear();
            break;
        }

        if (commitSudokuFlag.test_and_set()) {
            delete this;
            break;
        }
        else {
            commitSudokuFlag.clear();
        }
    }
}
