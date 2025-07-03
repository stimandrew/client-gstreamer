#pragma once
#include <QRunnable>
#include <gst/gst.h>

class SetPlaying : public QRunnable
{
public:
    SetPlaying(GstElement *);
    ~SetPlaying();

    void run ();

private:
    GstElement * pipeline_;
};

