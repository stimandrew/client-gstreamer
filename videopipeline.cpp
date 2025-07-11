#include "videopipeline.h"
#include <QDebug>
#include <gst/app/gstappsink.h>
#include "utils/common.h"
#include "utils/image_utils.h"
#include "utils/image_drawing.h"
#include <QPainter>

VideoPipeline::VideoPipeline(int port, QObject *parent)
    : QObject(parent), m_port(port)
{
    workerThread = new QThread();
    moveToThread(workerThread);
    workerThread->start();

    m_yoloProcessor = new YOLO11Processor();
    yoloThread = new QThread();
    m_yoloProcessor->moveToThread(yoloThread);
    connect(m_yoloProcessor, &YOLO11Processor::objectsDetected,
            this, &VideoPipeline::newObjects);
    yoloThread->start();
}

VideoPipeline::~VideoPipeline()
{
    stop();
    workerThread->quit();
    workerThread->wait();
    yoloThread->quit();
    yoloThread->wait();
    delete m_yoloProcessor;
}

void VideoPipeline::setYoloEnabled(bool enabled) {
    QMutexLocker locker(&m_pipelineMutex);
    m_yoloEnabled = enabled && m_yoloInitialized;
}

void VideoPipeline::setYoloModelPath(const QString& path) {
    QMutexLocker locker(&m_pipelineMutex);
    m_yoloInitialized = m_yoloProcessor->initialize(path.toStdString().c_str());
    if (!m_yoloInitialized) {
        qWarning() << "Failed to initialize YOLO model";
        m_yoloEnabled = false;
    }
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

    GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, "RGBA",
                                        nullptr);
    g_object_set(capsfilter, "caps", caps, nullptr);
    gst_caps_unref(caps);

    g_object_set(sink,
                 "emit-signals", TRUE,
                 "sync", FALSE,
                 "max-buffers", 1,
                 "drop", TRUE,
                 nullptr);
    g_signal_connect(sink, "new-sample", G_CALLBACK(newSampleCallback), this);

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
    queueMutex.lock();
    frameQueue.clear();
    queueMutex.unlock();
}

GstFlowReturn VideoPipeline::newSampleCallback(GstElement *sink, gpointer data)
{
    VideoPipeline *pipeline = static_cast<VideoPipeline*>(data);
    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    return pipeline->handleSample(sample);
}

GstFlowReturn VideoPipeline::handleSample(GstSample *sample) {

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


        QImage inputImage;
        if (format && strcmp(format, "NV12") == 0) {
            // Handle NV12 format
            inputImage = QImage(map.data, width, height, QImage::Format_RGBA8888);
        } else {
            // Fallback to RGB (you might need to add conversion)
            inputImage = QImage(map.data, width, height, QImage::Format_RGBA8888);
        }
        emit newFrame(inputImage.copy());

        if (m_yoloEnabled && m_yoloInitialized) {
            // 1. Конвертируем в RGB
            QImage yoloFrame = inputImage.convertToFormat(QImage::Format_RGB888);

            // 2. Масштабируем с сохранением пропорций
            float scale = qMin(640.0f / yoloFrame.width(), 640.0f / yoloFrame.height());
            int newWidth = yoloFrame.width() * scale;
            int newHeight = yoloFrame.height() * scale;
            yoloFrame = yoloFrame.scaled(newWidth, newHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

            // 3. Создаем изображение 640x640 с серой заливкой (114, 114, 114)
            QImage paddedImage(640, 640, QImage::Format_RGB888);
            paddedImage.fill(QColor(114, 114, 114));

            // 4. Центрируем масштабированное изображение
            QPainter painter(&paddedImage);
            int xOffset = (640 - yoloFrame.width()) / 2;
            int yOffset = (640 - yoloFrame.height()) / 2;
            painter.drawImage(xOffset, yOffset, yoloFrame);
            painter.end();

            // 5. Конвертируем данные в формат, ожидаемый моделью (NHWC)
            QImage finalImage = paddedImage.convertToFormat(QImage::Format_RGB888);

            // Отправляем на обработку в YOLO
            QMetaObject::invokeMethod(m_yoloProcessor, "processFrame",
                                      Qt::QueuedConnection,
                                      Q_ARG(QImage, finalImage));

            m_firstFrameProcessed = true;
        }

        gst_buffer_unmap(buffer, &map);
    }
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}
