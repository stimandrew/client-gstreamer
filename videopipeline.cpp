#include "videopipeline.h"
#include <QDebug>
#include <gst/app/gstappsink.h>
#include <QPainter>
#include <QDir>
#include <QDateTime>

VideoPipeline::VideoPipeline(int port, QObject *parent)
    : QObject(parent), m_port(port)
{
    workerThread = new QThread();
    moveToThread(workerThread);
    workerThread->start();

    yoloThread = new QThread();
    QObject::connect(yoloThread, &QThread::finished, yoloThread, &QObject::deleteLater);
    yoloThread->start();

}

VideoPipeline::~VideoPipeline()
{
    stop();
    if (m_yoloInitialized) {
        release_yolo11_model(&m_rknnAppCtx);
    }
    workerThread->quit();
    workerThread->wait();
    yoloThread->quit();
    yoloThread->wait();
}

void VideoPipeline::setYoloEnabled(bool enabled) {
    QMutexLocker locker(&m_pipelineMutex);
    m_yoloEnabled = enabled && m_yoloInitialized;
}

void VideoPipeline::setYoloModelPath(const QString& path) {
    QMutexLocker locker(&m_pipelineMutex);
    if (!path.isEmpty()) {
        int ret = init_yolo11_model(path.toStdString().c_str(), &m_rknnAppCtx);
        if (ret != 0) {
            qWarning() << "Failed to initialize YOLO model";
            m_yoloInitialized = false;
            m_yoloEnabled = false;
        } else {
            m_yoloInitialized = true;
            m_yoloEnabled = true;
        }
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
    if (!m_yoloInitialized) {
        qWarning() << "YOLO model not initialized";
        return;
    }

    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));

    QImage converted = frame.convertToFormat(QImage::Format_RGB888);
    src_image.width = converted.width();
    src_image.height = converted.height();
    src_image.format = IMAGE_FORMAT_RGB888;
    src_image.size = converted.width() * converted.height() * 3;
    src_image.virt_addr = (unsigned char*)malloc(src_image.size);
    memcpy(src_image.virt_addr, converted.bits(), src_image.size);

    object_detect_result_list od_results;
    memset(&od_results, 0, sizeof(od_results));

    int ret = inference_yolo11_model(&m_rknnAppCtx, &src_image, &od_results);
    if (ret != 0) {
        qWarning() << "YOLO inference failed";
        free(src_image.virt_addr);
        return;
    }

    // Создаем копию кадра для рисования bounding boxes
    QImage resultImage = frame.copy();
    QPainter painter(&resultImage);
    painter.setPen(QPen(Qt::red, 2));

    QList<QRect> objects;
    for (int i = 0; i < od_results.count; i++) {
        object_detect_result *det_result = &(od_results.results[i]);
        QRect rect(
            det_result->box.left,
            det_result->box.top,
            det_result->box.right - det_result->box.left,
            det_result->box.bottom - det_result->box.top
            );
        objects.append(rect);

        // Рисуем bounding box на изображении
        painter.drawRect(rect);

        // Выводим информацию об объекте в консоль
        qDebug() << "Detected object at:" << rect
                 << "Class ID:" << det_result->cls_id
                 << "Confidence:" << det_result->prop;
    }
    painter.end();

    // Сохраняем изображение с bounding boxes
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    QString outputDir = "detection_results";
    QDir().mkdir(outputDir); // Создаем директорию, если ее нет
    QString outputPath = QString("%1/detection_%2.jpg").arg(outputDir).arg(timestamp);

    if (resultImage.save(outputPath, "JPEG")) {
        qDebug() << "Saved detection results to:" << outputPath;
    } else {
        qWarning() << "Failed to save detection results";
    }

    emit newObjects(objects);
    free(src_image.virt_addr);
}
