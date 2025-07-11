#pragma once

#include <QObject>
#include <QThread>
#include <QImage>
#include <QMutex>
#include <QQueue>
#include <gst/gst.h>
#include "yolo11.h"

class VideoPipeline : public QObject
{
    Q_OBJECT
public:
    explicit VideoPipeline(int port, QObject *parent = nullptr);
    ~VideoPipeline();

    bool initialize();
    void start();
    void stop();
    void setYoloEnabled(bool enabled);
    void setYoloModelPath(const QString& path);

signals:
    void newFrame(const QImage &frame);
    void newObjects(const QList<QRect> &objects);

private:
    static GstFlowReturn newSampleCallback(GstElement *sink, gpointer data);
    GstFlowReturn handleSample(GstSample *sample);

    GstElement *pipeline = nullptr;
    GstElement *sink = nullptr;
    int m_port;
    QThread *workerThread;
    QThread *yoloThread;
    mutable QMutex m_pipelineMutex;
    YOLO11Processor* m_yoloProcessor = nullptr;
    bool m_yoloEnabled = false;
    bool m_yoloInitialized = false;
    QQueue<QImage> frameQueue;
    QMutex queueMutex;
    bool m_firstFrameProcessed = false;
};
