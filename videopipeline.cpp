#include "videopipeline.h"
#include <QQuickItem>
#include <QQuickWindow>
#include <gst/gst.h>

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
    if (glContext) {
        glContext->makeCurrent(surface);
        glContext->doneCurrent();
        delete glContext;
    }
    if (surface) {
        delete surface;
    }
    workerThread->quit();
    workerThread->wait();
}

bool VideoPipeline::initialize()
{
    // Инициализация OpenGL контекста с правильными параметрами
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGLES);
    format.setVersion(2, 0);
    format.setProfile(QSurfaceFormat::NoProfile);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);

    glContext = new QOpenGLContext();
    glContext->setFormat(format);

    if (!glContext->create()) {
        qWarning("Failed to create OpenGL context");
        return false;
    }

    surface = new QOffscreenSurface();
    surface->setFormat(glContext->format());
    surface->create();

    // Инициализация GStreamer GL
    if (!gst_is_initialized()) {
        gst_init(nullptr, nullptr);
    }

    // Создаем элементы pipeline с проверкой
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
    g_object_set(jitterbuffer,
                 "latency", 200,
                 "do-lost", TRUE,
                 "drop-on-latency", TRUE,
                 nullptr);

    g_object_set(queue,
                 "max-size-buffers", 2,
                 "max-size-time", 0,
                 "max-size-bytes", 0,
                 "leaky", 2, // 2 = downstream
                 nullptr);

    g_object_set(sink,
                 "sync", FALSE,
                 "async", FALSE,
                 "max-lateness", -1,
                 "qos", FALSE,
                 nullptr);

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

void VideoPipeline::start() {
    QMutexLocker locker(&m_pipelineMutex);
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_READY);
        GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            qWarning("Failed to start pipeline");
        }
    }
}

void VideoPipeline::stop() {
    QMutexLocker locker(&m_pipelineMutex);
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
    }
}

void VideoPipeline::setVideoItem(QObject *videoItem)
{
    if (sink && videoItem) {
        if (glContext->makeCurrent(surface)) {
            g_object_set(sink, "widget", videoItem, nullptr);
            glContext->doneCurrent();
        } else {
            qWarning("Failed to make OpenGL context current");
        }
    }
}
