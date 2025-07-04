#pragma once

#include <QObject>
#include <gst/gst.h>
#include "setplaying.h"

class SetPlaying;

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

    QRunnable* createSetPlayingJob();
    int port() const { return m_port; }

private:
    GstElement *pipeline = nullptr;
    GstElement *sink = nullptr;
    int m_port;
};
