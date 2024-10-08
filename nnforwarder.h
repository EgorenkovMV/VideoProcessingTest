#ifndef NNFORWARDER_H
#define NNFORWARDER_H

#include <QObject>
#include <QImage>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/dnn/all_layers.hpp>


class NNForwarder : public QObject
{
    Q_OBJECT
public:
    explicit NNForwarder(QObject *parent = nullptr);

    QImage forward(const QImage &frame);
    void initVideoToFileWriter(QSize frameSize);
    void setFps(float fps = 30);

private:
    cv::dnn::Net model;
    std::vector<std::string> class_names;

    cv::VideoWriter videoToFileWriter;
    bool videoToFileInited = false;
    float fps;
};

#endif // NNFORWARDER_H
