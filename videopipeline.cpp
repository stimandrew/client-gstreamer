#include "videopipeline.h"
#include <QDebug>
#include <gst/app/gstappsink.h>

VideoPipeline::VideoPipeline(int port, QObject *parent)
    : QObject(parent), m_port(port)
{
    workerThread = new QThread(this);
    moveToThread(workerThread);
    workerThread->start();
}

VideoPipeline::~VideoPipeline()
{
    stop();
    workerThread->quit();
    workerThread->wait();
}

bool VideoPipeline::initialize()
{
    pipeline = gst_pipeline_new("video-pipeline");
    GstElement *src = gst_element_factory_make("udpsrc", nullptr);
    GstElement *jitterbuffer = gst_element_factory_make("rtpjitterbuffer", nullptr);
    GstElement *rtpdepay = gst_element_factory_make("rtph264depay", nullptr);
    GstElement *parse = gst_element_factory_make("h264parse", nullptr);
    GstElement *identity = gst_element_factory_make("identity", nullptr);
    GstElement *decoder = gst_element_factory_make("mppvideodec", nullptr);
    GstElement *queue = gst_element_factory_make("queue", nullptr);
    GstElement *convert = gst_element_factory_make("videoconvert", nullptr);
    GstElement *capsfilter = gst_element_factory_make("capsfilter", nullptr);
    sink = gst_element_factory_make("appsink", nullptr);

    if (!src || !jitterbuffer || !rtpdepay || !parse || !identity || !decoder ||
        !queue || !convert || !capsfilter || !sink) {
        qWarning() << "Failed to create elements";
        return false;
    }

    // Настройка элементов
    g_object_set(src, "port", m_port,
                 "caps", gst_caps_from_string("application/x-rtp,media=video,encoding-name=H264"),
                 nullptr);
    g_object_set(jitterbuffer,
                 "latency", 200,
                 "do-lost", TRUE,
                 "drop-on-latency", TRUE,
                 nullptr);

    g_object_set(queue,
                 "max-size-buffers", 2,
                 "max-size-time", 0,
                 "max-size-bytes", 0,
                 "leaky", 2,
                 nullptr);

    // Настройка caps для вывода в RGB формате
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, "RGBA",
                                        nullptr);
    g_object_set(capsfilter, "caps", caps, nullptr);
    gst_caps_unref(caps);

    // Настройка appsink
    g_object_set(sink,
                 "emit-signals", TRUE,
                 "sync", FALSE,
                 nullptr);
    g_signal_connect(sink, "new-sample", G_CALLBACK(newSampleCallback), this);

    // Добавляем элементы в pipeline
    gst_bin_add_many(GST_BIN(pipeline),
                     src,
                     jitterbuffer,
                     rtpdepay,
                     parse,
                     identity,
                     decoder,
                     queue,
                     convert,
                     capsfilter,
                     sink,
                     nullptr);

    // Соединяем элементы
    if (!gst_element_link_many(src, jitterbuffer, rtpdepay, parse, identity, decoder,
                               queue, convert, capsfilter, sink, nullptr)) {
        qWarning() << "Failed to link elements";
        return false;
    }

    return true;
}

void VideoPipeline::start()
{
    QMutexLocker locker(&m_pipelineMutex);
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_READY);
        GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            qWarning("Failed to start pipeline");
        }
    }
}

void VideoPipeline::stop()
{
    QMutexLocker locker(&m_pipelineMutex);
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
    }
}

GstFlowReturn VideoPipeline::newSampleCallback(GstElement *sink, gpointer data)
{
    VideoPipeline *pipeline = static_cast<VideoPipeline*>(data);
    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    return pipeline->handleSample(sample);
}

GstFlowReturn VideoPipeline::handleSample(GstSample *sample)
{
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *structure = gst_caps_get_structure(caps, 0);

    int width, height;
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);

    const char *format;
    format = gst_structure_get_string(structure, "format");

    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        QImage image;
        if (format && strcmp(format, "NV12") == 0) {
            // Handle NV12 format
            image = QImage(map.data, width, height, QImage::Format_RGBA8888);
        } else {
            // Fallback to RGB (you might need to add conversion)
            image = QImage(map.data, width, height, QImage::Format_RGBA8888);
        }
        emit newFrame(image.copy());
        gst_buffer_unmap(buffer, &map);
    }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}
