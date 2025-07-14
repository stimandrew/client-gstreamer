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
        QImage image;
        if (format && strcmp(format, "NV12") == 0) {
            // Handle NV12 format
            image = QImage(map.data, width, height, QImage::Format_RGBA8888);
        } else {
            // Fallback to RGB (you might need to add conversion)
            image = QImage(map.data, width, height, QImage::Format_RGBA8888);
        }
        emit newFrame(image.copy());

        if (m_yoloEnabled && m_yoloInitialized && !m_firstFrameProcessed) {
            processFrameWithRGA(image);
            m_firstFrameProcessed = true;
        }

        gst_buffer_unmap(buffer, &map);
    }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

void VideoPipeline::processFrameWithRGA(const QImage &frame)
{
    // Конвертируем в RGB888 и выравниваем размеры
    QImage rgbFrame = frame.convertToFormat(QImage::Format_RGB888);


    if (rgbFrame.isNull()) {
        qWarning() << "Failed to convert frame to RGB888";
        return;
    }

    image_buffer_t src_img;
    src_img.width = rgbFrame.width();
    src_img.height = rgbFrame.height();
    src_img.format = IMAGE_FORMAT_RGB888;
    src_img.size = rgbFrame.sizeInBytes();
    src_img.virt_addr = const_cast<unsigned char*>(rgbFrame.bits());

    image_buffer_t dst_img;
    dst_img.width = 640;
    dst_img.height = 640;
    dst_img.format = IMAGE_FORMAT_RGB888;
    dst_img.size = 640 * 640 * 3;
    dst_img.virt_addr = static_cast<unsigned char*>(malloc(dst_img.size));

    qDebug() << "Source image:" << src_img.width << "x" << src_img.height
             << "format:" << src_img.format;
    qDebug() << "Destination image:" << dst_img.width << "x" << dst_img.height
             << "format:" << dst_img.format;

    if (!dst_img.virt_addr) {
        qWarning() << "Failed to allocate memory for YOLO input";
        return;
    }

    // Заполняем фон серым цветом (114) перед обработкой
    memset(dst_img.virt_addr, 114, dst_img.size);

    letterbox_t letter_box;
    int ret = convert_image_with_letterbox(&src_img, &dst_img, &letter_box, 114);
    if (ret != 0) {
        qWarning() << "RGA image conversion failed:" << ret;
        free(dst_img.virt_addr);
        return;
    }

    QImage yoloImage(dst_img.virt_addr, dst_img.width, dst_img.height,
                     QImage::Format_RGB888, [](void *ptr){ free(ptr); }, dst_img.virt_addr);

    QMetaObject::invokeMethod(m_yoloProcessor, "processFrame",
                              Qt::QueuedConnection,
                              Q_ARG(QImage, yoloImage.copy()));
}
