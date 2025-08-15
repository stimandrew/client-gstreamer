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

    m_yoloTimer = new QTimer(this);
    m_yoloTimer->setInterval(1000); // 1 секунда
    m_yoloTimer->moveToThread(yoloThread);
    QObject::connect(m_yoloTimer, &QTimer::timeout, this, &VideoPipeline::processNextFrame);

    m_fpsTimer.start();
    // Инициализация мьютексов
    m_rknnMutexes.push_back(std::make_unique<QMutex>());
    m_rknnMutexes.push_back(std::make_unique<QMutex>());
}

VideoPipeline::~VideoPipeline()
{
    stop();
    for (int i = 0; i < 2; ++i) {
        if (m_yoloInitialized) {
            release_yolo11_model(&m_rknnAppCtx[i]);
        }
    }
    workerThread->quit();
    workerThread->wait();
    yoloThread->quit();
    yoloThread->wait();
}

void VideoPipeline::processNextFrame()
{
    if (!frameQueue.isEmpty()) {
        QImage frame;
        queueMutex.lock();
        frame = frameQueue.dequeue();
        frameQueue.clear(); // Обрабатываем только последний кадр
        queueMutex.unlock();
        processFrameWithRGA(frame);
    }
}

void VideoPipeline::setYoloEnabled(bool enabled) {
    QMutexLocker locker(&m_pipelineMutex);
    m_yoloEnabled = enabled && m_yoloInitialized;
    if (m_yoloEnabled) {
        QMetaObject::invokeMethod(m_yoloTimer, "start");
    } else {
        QMetaObject::invokeMethod(m_yoloTimer, "stop");
        queueMutex.lock();
        frameQueue.clear();
        queueMutex.unlock();
    }
}

void VideoPipeline::setYoloModelPath(const QString& path) {
    if (!path.isEmpty()) {
        init_post_process();
        for (int i = 0; i < 2; ++i) {
            std::unique_lock<QMutex> lock(*m_rknnMutexes[i]); // Используем unique_lock
            int ret = init_yolo11_model(path.toStdString().c_str(), &m_rknnAppCtx[i]);
            if (ret != 0) {
                qWarning() << "Failed to initialize YOLO model for core" << i;
                m_yoloInitialized = false;
                return;
            }
            rknn_set_core_mask(m_rknnAppCtx[i].rknn_ctx,
                               (i == 0) ? RKNN_NPU_CORE_0 : RKNN_NPU_CORE_1);
        }
        m_yoloInitialized = true;
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

int VideoPipeline::fps() const
{
    return m_fps;
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

    // Обновляем счётчик FPS
    m_frameCount++;
    if (m_fpsTimer.elapsed() >= 1000) { // Каждую секунду
        m_fps = m_frameCount;
        m_frameCount = 0;
        m_fpsTimer.restart();
        emit fpsChanged(m_fps);
    }

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

        if (m_yoloEnabled && m_yoloInitialized) {
            processFrameWithRGA(image);
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

    static std::atomic<int> thread_counter{0};
    int thread_id = thread_counter.fetch_add(1) % 2;

    std::unique_lock<QMutex> lock(*m_rknnMutexes[thread_id]); // Используем unique_lock

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

    int ret = inference_yolo11_model(&m_rknnAppCtx[thread_id], &src_image, &od_results);
    if (ret != 0) {
        qWarning() << "YOLO inference failed";
        free(src_image.virt_addr);
        return;
    }

    QList<QPair<QRect, QString>> objects; // Пара: прямоугольник и метка класса
    for (int i = 0; i < od_results.count; i++) {
        object_detect_result *det_result = &(od_results.results[i]);
        QRect rect(
            det_result->box.left,
            det_result->box.top,
            det_result->box.right - det_result->box.left,
            det_result->box.bottom - det_result->box.top
            );

        // Получаем название класса
        const char* cls_name = coco_cls_to_name(det_result->cls_id);
        QString label = QString("%1 %2%").arg(QString::fromUtf8(cls_name))
                            .arg(QString::number(det_result->prop * 100, 'f', 0));

        objects.append(qMakePair(rect, label));

        qDebug() << "Detected:" << label << "at:" << rect;
    }

    emit newObjects(objects);
    free(src_image.virt_addr);
}
