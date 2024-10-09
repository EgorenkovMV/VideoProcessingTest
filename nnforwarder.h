#ifndef NNFORWARDER_H
#define NNFORWARDER_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QJsonDocument>
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

    float fps;
    QString dirPath;

private:
    cv::dnn::Net model;
    std::vector<std::string> class_names;

    cv::VideoWriter videoToFileWriter;
    bool videoToFileInited = false;

    QJsonDocument metadata;
};

#endif // NNFORWARDER_H
