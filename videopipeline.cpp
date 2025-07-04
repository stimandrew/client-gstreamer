#include "videopipeline.h"
#include <QQuickItem>
#include <gst/gst.h>

VideoPipeline::VideoPipeline(int port, QObject *parent) : QObject(parent), m_port(port) {}

VideoPipeline::~VideoPipeline()
{
    stop();
    if (pipeline) {
        gst_object_unref(pipeline);
    }
}

bool VideoPipeline::initialize()
{
    pipeline = gst_pipeline_new("video-pipeline");

    // Создаем элементы pipeline
    GstElement *src = gst_element_factory_make("udpsrc", nullptr);
    GstElement *jitterbuffer = gst_element_factory_make("rtpjitterbuffer", nullptr);
    GstElement *rtpdepay = gst_element_factory_make("rtph264depay", nullptr);
    GstElement *parse = gst_element_factory_make("h264parse", nullptr);
    GstElement *decoder = gst_element_factory_make("mppvideodec", nullptr);
    GstElement *queue = gst_element_factory_make("queue", nullptr);
    GstElement *glupload = gst_element_factory_make("glupload", nullptr);
    GstElement *convert = gst_element_factory_make("glcolorconvert", nullptr);
    sink = gst_element_factory_make("qml6glsink", nullptr);

    if (!src || !jitterbuffer || !rtpdepay || !parse || !decoder ||
        !queue || !glupload || !convert || !sink) {
        return false;
    }

    // Настраиваем элементы
    g_object_set(src, "port", m_port, // Используем переданный порт
                 "caps", gst_caps_from_string("application/x-rtp,media=video,encoding-name=H264"),
                 nullptr);
    g_object_set(jitterbuffer, "latency", 200, nullptr);
    g_object_set(queue, "max-size-buffers", 3, nullptr);
    g_object_set(sink, "sync", FALSE, nullptr);

    // Добавляем элементы в pipeline
    gst_bin_add_many(GST_BIN(pipeline),
                     src,
                     jitterbuffer,
                     rtpdepay,
                     parse,
                     decoder,
                     queue,
                     glupload,
                     convert,
                     sink,
                     nullptr);

    // Соединяем элементы
    if (!gst_element_link_many(src, jitterbuffer, rtpdepay, parse, decoder,
                               queue, glupload, convert, sink, nullptr)) {
        return false;
    }

    return true;
}

void VideoPipeline::start()
{
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
    }
}

void VideoPipeline::stop()
{
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
    }
}

void VideoPipeline::setVideoItem(QObject *videoItem)
{
    if (sink && videoItem) {
        g_object_set(sink, "widget", videoItem, nullptr);
    }
}

QRunnable* VideoPipeline::createSetPlayingJob()
{
    return new SetPlaying(pipeline ? static_cast<GstElement*>(gst_object_ref(pipeline)) : nullptr);
}
