#include "tcpsender.h"
#include <memory>
#include <QImage>
#include <boost/crc.hpp>


TcpSender::TcpSender(QObject *parent)
    : QObject{parent}
    , ser {nullptr}
    , soc {nullptr}
{
    ser = new QTcpServer(this);

    frameRGB = av_frame_alloc();
    frameYUV = av_frame_alloc();
    isSetUp = false;


    connect(ser, &QTcpServer::newConnection, this, &TcpSender::acceptConnection);

}

TcpSender::~TcpSender()
{
    pause();
    if (soc != nullptr) {
        soc->disconnectFromHost();
        soc->waitForDisconnected(200);
        delete soc;
        soc = nullptr;
    }
    if (ser != nullptr) {
        delete ser;
        ser = nullptr;
    }

    delete [] frameRGBBuff;
    av_frame_free(&frameRGB);
    av_frame_free(&frameYUV);
    av_packet_free(&packetOut);
    if (imgConvertCtx) {
        sws_freeContext(imgConvertCtx);
    }
}

void TcpSender::establishConnection(const QHostAddress &address, quint16 port)
{
    if (soc != nullptr) {
        soc->disconnectFromHost();
        soc->waitForDisconnected(200);
        delete soc;
        soc = nullptr;
    }


    soc = new QTcpSocket(this);
    soc->connectToHost(address, port);
    setupSocket();
}

void TcpSender::listenToPort(int port)
{
    if (ser == nullptr) {
        return;
    }
    if (ser->isListening()) {
        ser->close();
    }
    ser->listen(QHostAddress::Any, port);
}

void TcpSender::acceptConnection()
{
    if (soc != nullptr) {
        soc->disconnectFromHost();
        soc->waitForDisconnected(200);
        delete soc;
    }

    soc = ser->nextPendingConnection();
    setupSocket();

    qDebug() << "Server accepted new connection" << soc->localAddress() << "port" << soc->localPort();
}

void TcpSender::setupSocket()
{
    connect(soc, &QTcpSocket::connected, [this] () {
        qDebug() << "Socket connected to" << soc->localAddress() << "port" << soc->localPort();
        emit connected(soc->localAddress(), soc->localPort());
    });
    connect(soc, &QTcpSocket::readyRead, this, &TcpSender::readPackage);
    connect(soc, &QTcpSocket::disconnected, this, &TcpSender::disconnected);
    connect(soc, &QTcpSocket::stateChanged, [this] () {
        qDebug() << "now socket state is" << soc->state();
    });
    connect(soc, &QTcpSocket::errorOccurred, [this] () {
        qDebug() << "socket error occured" << soc->error();
    });
}


void TcpSender::setup(const QImage &frame)
{
    codec = avcodec_find_encoder(AVCodecID::AV_CODEC_ID_H264);
    if (!codec) {
        qDebug() << "codec not found";
        exit(1);
    }

    context = avcodec_alloc_context3(codec);

    qDebug() << "\n\tTcpSender::setup with size:" << frame.size();

    context->width = frame.width();
    context->height = frame.height();
    context->time_base = (AVRational){1, 25};
    context->framerate = (AVRational){25, 1};
    context->gop_size = 10;
    context->max_b_frames = 1;
    context->pix_fmt = AVPixelFormat::AV_PIX_FMT_YUV420P;
    context->bit_rate = 1'600'000;


    if (avcodec_open2(context, codec, nullptr) < 0) {
        qDebug() << "could not open codec";
        exit(1);
    }

    frameCounter = 0;


    frameRGB->format = AVPixelFormat::AV_PIX_FMT_RGB24;
    frameRGB->width = context->width;
    frameRGB->height = context->height;

    frameYUV->format = AVPixelFormat::AV_PIX_FMT_YUV420P;
    frameYUV->width = context->width;
    frameYUV->height = context->height;
    if (av_frame_get_buffer(frameYUV, 0) < 0) {
        qDebug() << "could not get buffer for frameYUV";
        exit(1);
    }

    frameRGBBuff = new uchar [frame.width() * frame.height() * 3] ();

    imgConvertCtx = sws_getCachedContext(imgConvertCtx,
                                         context->width, context->height,
                                         AVPixelFormat::AV_PIX_FMT_RGB24,
                                         context->width, context->height,
                                         AVPixelFormat::AV_PIX_FMT_YUV420P,
                                         SWS_BICUBIC, nullptr, nullptr, nullptr);

    isSetUp = true;
}

void TcpSender::sendFrame(const QImage &frame)
{
    if (soc == nullptr) {
        return;
    }

    qDebug() << "\n\tStarted sending frame" << frameCounter;
    if (not isSetUp) {
        setup(frame);
    }

    av_packet_free(&packetOut);
    packetOut = av_packet_alloc();

    frameYUV->pts = frameCounter;
    frameCounter++;

    std::memcpy(frameRGBBuff, frame.constBits(), frame.width() * frame.height() * 3);

    frameRGB->data[0] = frameRGBBuff;
    frameRGB->linesize[0] = context->width * 3;


    sws_scale(imgConvertCtx, frameRGB->data, frameRGB->linesize, 0, context->height, frameYUV->data, frameYUV->linesize);


    qDebug() << "encoding frame: avcodec_send_frame" << avcodec_send_frame(context, frameYUV);

    int result = 0;
    while (result >= 0) {
        result = avcodec_receive_packet(context, packetOut);
        qDebug() << "encoding frame: avcodec_receive_packet" << result;

        if (result == 0) {
            sendPackage(packetOut);
        }
    }
}

void TcpSender::pause()
{
    isSetUp = false;

    if (not isSetUp) {
        return;
    }

    int result = 0;
    while (result >= 0) {
        result = avcodec_send_frame(context, nullptr);
        qDebug() << "\n\tTcpSender::pause: flush" << result;
    }

    avcodec_free_context(&context);
}

void TcpSender::readPackage()
{
    /// Package structure:
    /// 0xf0, 0x0f, packet->size, packet->data, crc32, 0xff, 0xff
    ///   1     1         8            any        4      1     1  bytes
    int minPackageSize = 16;

    QList<uint8_t> syncBytesHead {0xf0, 0x0f};
    QList<uint8_t> syncBytesTail {0xff, 0xff};


    readBuffer.append(soc->readAll());

    forever {
        int headPos = readBuffer.indexOf(QByteArrayView {syncBytesHead});
        if (headPos == -1) {
            qDebug() << "No head. Finalizing...";
            break;
        }


        quint64 dataSize;
        if (readBuffer.size() >= headPos + minPackageSize) {
            QDataStream ds {readBuffer.mid(headPos + 2, 8)};
            ds.setByteOrder(QDataStream::BigEndian);
            ds >> dataSize;
            qDebug() << "Head found. Size:" << dataSize;
        }
        else {
            qDebug() << "Head found, but received data is too short. Finalizing...";
            break;
        }

        if (readBuffer.size() < headPos + minPackageSize + dataSize) {
            qDebug() << "Package of insufficient length. Finalizing...";
            break;
        }

        qDebug() << "Full package received. Processing...";

        QByteArray package = readBuffer.mid(headPos + 2 + 8, headPos + dataSize + 4);
        readBuffer.remove(0, headPos + minPackageSize + dataSize);

        // owner of packet must unref it's data
        AVPacket *packetIn = av_packet_alloc();

        quint32 crc32Received;
        boost::crc_32_type crc32Calculated;

        QDataStream ds {package};
        ds.setByteOrder(QDataStream::BigEndian);

        av_new_packet(packetIn, dataSize);
        packetIn->size = dataSize;

        ds >> packetIn >> crc32Received;

        crc32Calculated.process_bytes(packetIn->data, packetIn->size);

        if (crc32Calculated.checksum() != crc32Received) {
            qDebug() << "CRC32 doesnt match.";
            continue;
        }

        qDebug() << "Package read successfully.";
        emit receivedPacket(packetIn);
        av_packet_free(&packetIn);    ///???
    }
}

void TcpSender::sendPackage(AVPacket *packet)
{
    if (soc == nullptr) {
        return;
    }
    if (soc->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    QDataStream ds {soc};
    ds.setByteOrder(QDataStream::BigEndian);

    uint8_t syncBytesHead [2] {0xf0, 0x0f};
    quint64 dataSize = packet->size;
    boost::crc_32_type crc32;
    uint8_t syncBytesTail [2] {0xff, 0xff};

    crc32.process_bytes(packet->data, packet->size);

    ds << syncBytesHead[0] << syncBytesHead[1] <<
          dataSize << packet << static_cast<quint32>(crc32.checksum()) <<
          syncBytesTail[0] << syncBytesTail[1];

    qDebug() << "Package sent successfully. Data size:" << packet->size + 16 <<
                "crc32" << static_cast<quint32>(crc32.checksum());
}


QDataStream &operator<<(QDataStream &out, const AVPacket *packet)
{
    for (int byte = 0; byte < packet->size; byte++) {
        out << packet->data[byte];
    }

    return out;
}


QDataStream &operator>>(QDataStream &in, AVPacket *packet)
{
    for (int byte = 0; byte < packet->size; byte++) {
        in >> packet->data[byte];
    }


    return in;
}

