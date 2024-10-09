#ifndef STREAMINTERFACE_H
#define STREAMINTERFACE_H

#include <QObject>
#include <QImage>
#include <QMutex>
#include <QDebug>
#include <atomic>
#include <chrono>

struct PerformanceAnalyzer
{
    std::chrono::high_resolution_clock::time_point startingTime;
    long int usecDutationSum = 0;
    long int count = -1;
    bool mute = true;    // debug


    void setStartingTime() { startingTime = std::chrono::high_resolution_clock::now(); }
    void makeNote(const QString &whatHappened)
    {
        if (mute) return;
        qDebug() << (std::chrono::high_resolution_clock::now() - startingTime).count() / 1000
                 << "usec since start happened:" << whatHappened;
    }
    void calcAvg()
    {
        count++;
        usecDutationSum += (std::chrono::high_resolution_clock::now() - startingTime).count() / 1000;
        if (count > 0 and not mute) {
            qDebug() << "curr avg frame process time (usec):" << usecDutationSum / count;
        }
    }
};


class StreamInterface : public QObject
{
    Q_OBJECT
public:
    explicit StreamInterface(QObject *parent = nullptr) : QObject {parent} { }
    virtual ~StreamInterface() { }
    virtual void startStreamingRoutine() = 0;
    virtual void stop() = 0;
    virtual QImage getLastFrame() = 0;

    std::atomic_flag stopFlag;
    PerformanceAnalyzer pa;

signals:
    void frameReady();
    void nativeFrameReady();

protected:
    QImage lastFrame;


};

#endif // STREAMINTERFACE_H
