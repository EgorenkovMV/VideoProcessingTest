#ifndef TCPSENDER_H
#define TCPSENDER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVideoFrame>


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

class TcpSender : public QObject
{
    Q_OBJECT
public:
    explicit TcpSender(QObject *parent = nullptr);
    ~TcpSender();

signals:
    void newConnection();
    void connected(const QHostAddress &addr, quint16 port);
    void disconnected();
    void receivedPacket(AVPacket *packet);

public slots:
    void establishConnection(const QHostAddress &address, quint16 port);
    void sendFrame(const QImage &frame);
    void pause();
    void listenToPort(int port);


private slots:
    void acceptConnection();
    void readPackage();

private:
    QTcpServer *ser;
    QTcpSocket *soc;

    QByteArray readBuffer;

    int frameCounter          = 0;
    AVCodec *codec            = nullptr;
    AVCodecContext *context   = nullptr;
    AVFrame *frameRGB         = nullptr;
    AVFrame *frameYUV         = nullptr;
    uchar *frameRGBBuff       = nullptr;
    AVPacket *packetOut       = nullptr;
    SwsContext *imgConvertCtx = nullptr;

    bool isSetUp = false;


    void setupSocket();
    void setup(const QImage &frame);
    void sendPackage(AVPacket *packet);

};


QDataStream &operator<<(QDataStream &out, const AVPacket *packet);
QDataStream &operator>>(QDataStream &in, AVPacket *packet);

#endif // TCPSENDER_H
