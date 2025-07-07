#pragma once

#include <QObject>
#include <QThread>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <gst/gst.h>
#include <QMutex>

class VideoPipeline : public QObject
{
    Q_OBJECT
public:
    explicit VideoPipeline(int port, QObject *parent = nullptr);
    ~VideoPipeline();

    bool initialize();
    void start();
    void stop();
    void setVideoItem(QObject *videoItem);

    int port() const { return m_port; }
    GstElement* getPipeline() const { return pipeline; }

private:
    GstElement *pipeline = nullptr;
    GstElement *sink = nullptr;
    int m_port;
    QThread *workerThread;
    QOpenGLContext *glContext = nullptr;
    QOffscreenSurface *surface = nullptr;
    mutable QMutex m_pipelineMutex;
};
