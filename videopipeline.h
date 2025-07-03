#pragma once

#include <QObject>
#include <gst/gst.h>
#include "setplaying.h"

class SetPlaying;

class VideoPipeline : public QObject
{
    Q_OBJECT
public:
    explicit VideoPipeline(QObject *parent = nullptr);
    ~VideoPipeline();

    bool initialize();
    void start();
    void stop();
    void setVideoItem(QObject *videoItem);

    QRunnable* createSetPlayingJob();

private:
    GstElement *pipeline = nullptr;
    GstElement *sink = nullptr;
};
