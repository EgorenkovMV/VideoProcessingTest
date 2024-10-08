#include "nnforwarder.h"
#include <iostream>
#include <fstream>


NNForwarder::NNForwarder(QObject *parent)
    : QObject{parent}
{
    std::ifstream ifs(std::string("../pretrained_models/object_detection_classes_coco.txt").c_str());
    std::string line;
    while (getline(ifs, line))
    {
        class_names.push_back(line);
    }

    // load the neural network model
    model = cv::dnn::readNet("../pretrained_models/frozen_inference_graph.pb",
                             "../pretrained_models/ssd_mobilenet_v2_coco_2018_03_29.pbtxt.txt", "TensorFlow");

    model.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
    model.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);

}

QImage NNForwarder::forward(const QImage &frame)
{
    if (not videoToFileInited) {
        initVideoToFileWriter(frame.size());
    }

    cv::Mat cvFrame {frame.height(), frame.width(), CV_8UC3, (cv::Scalar*)frame.scanLine(0)};

    // create blob from image
    cv::Mat blob = cv::dnn::blobFromImage(cvFrame, 1.0, cv::Size(300, 300),
                                          cv::Scalar(127.5, 127.5, 127.5), true, false);
    model.setInput(blob);
    // forward pass through the model to carry out the detection
    cv::Mat output = model.forward();

    cv::Mat detectionMat(output.size[2], output.size[3], CV_32F, output.ptr<float>());

    for (int i = 0; i < detectionMat.rows; i++){
        int class_id = detectionMat.at<float>(i, 1);
        float confidence = detectionMat.at<float>(i, 2);

        // check if the detection is of good quality
        if (confidence > 0.4){
            int box_x = static_cast<int>(detectionMat.at<float>(i, 3) * cvFrame.cols);
            int box_y = static_cast<int>(detectionMat.at<float>(i, 4) * cvFrame.rows);
            int box_width = static_cast<int>(detectionMat.at<float>(i, 5) * cvFrame.cols - box_x);
            int box_height = static_cast<int>(detectionMat.at<float>(i, 6) * cvFrame.rows - box_y);
            cv::rectangle(cvFrame, cv::Point(box_x, box_y), cv::Point(box_x+box_width, box_y+box_height), cv::Scalar(255,255,255), 2);
            cv::putText(cvFrame, class_names[class_id-1].c_str(), cv::Point(box_x, box_y-5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,255,255), 1);
        }
    }

    if (videoToFileInited) {
        videoToFileWriter.write(cvFrame);
    }

    QImage frameOut = QImage((uchar*) cvFrame.data, cvFrame.cols, cvFrame.rows, cvFrame.step, QImage::Format_RGB888);

    return frameOut;

}

void NNForwarder::initVideoToFileWriter(QSize frameSize)
{
    // create the `VideoWriter()` object
    videoToFileWriter = cv::VideoWriter {"video_result.avi", cv::VideoWriter::fourcc('H', '2', '6', '4'),
                                         fps, cv::Size(frameSize.width(), frameSize.height())};

    videoToFileInited = true;
}

void NNForwarder::setFps(float fps)
{
    this->fps = fps;
}
