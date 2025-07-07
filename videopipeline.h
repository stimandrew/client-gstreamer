#pragma once

#include <QObject>
#include <QThread>
#include <QImage>
#include <QMutex>
#include <gst/gst.h>

class VideoPipeline : public QObject
{
    Q_OBJECT
public:
    explicit VideoPipeline(int port, QObject *parent = nullptr);
    ~VideoPipeline();

    bool initialize();
    void start();
    void stop();

signals:
    void newFrame(const QImage &frame);

private:
    static GstFlowReturn newSampleCallback(GstElement *sink, gpointer data);
    GstFlowReturn handleSample(GstSample *sample);

    GstElement *pipeline = nullptr;
    GstElement *sink = nullptr;
    int m_port;
    QThread *workerThread;
    mutable QMutex m_pipelineMutex;
};
