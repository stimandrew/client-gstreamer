#pragma once

#include <QObject>
#include <QThread>
#include <QImage>
#include <QMutex>
#include <QQueue>
#include <QTimer>
#include <QElapsedTimer>
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

private slots:
    void processNextFrame();

private:
    static GstFlowReturn newSampleCallback(GstElement *sink, gpointer data);
    GstFlowReturn handleSample(GstSample *sample);
    void processFrameWithRGA(const QImage &frame);
    rknn_app_context_t m_rknnAppCtx;

    GstElement *pipeline = nullptr;
    GstElement *sink = nullptr;
    int m_port;
    QThread *workerThread;
    QThread *yoloThread;
    QTimer* m_yoloTimer;
    mutable QMutex m_pipelineMutex;
    bool m_yoloEnabled = false;
    bool m_yoloInitialized = false;
    QQueue<QImage> frameQueue;
    QMutex queueMutex;
    QElapsedTimer m_frameTimer;
    int m_frameCount = 0;
};
