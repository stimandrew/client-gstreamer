#include "videocontroller.h"
#include <QDebug>

VideoController::VideoController(QObject *parent) : QObject(parent)
{
}

VideoController::~VideoController()
{
    stop();
    delete m_pipeline;
}

bool VideoController::isRunning() const
{
    return m_isRunning;
}

int VideoController::port() const
{
    return m_port;
}

bool VideoController::yoloEnabled() const { return m_yoloEnabled; }
QString VideoController::yoloModelPath() const { return m_yoloModelPath; }

void VideoController::setPort(int port)
{
    if (m_port != port) {
        resetPipeline();
        m_port = port;
        emit portChanged(port);
    }
}

void VideoController::setYoloEnabled(bool enabled) {
    if (m_yoloEnabled != enabled) {
        m_yoloEnabled = enabled;
        if (m_pipeline) {
            m_pipeline->setYoloEnabled(enabled);
        }
        emit yoloEnabledChanged(enabled);
    }
}

void VideoController::setYoloModelPath(const QString& path) {
    if (m_yoloModelPath != path) {
        m_yoloModelPath = path;
        if (m_pipeline) {
            m_pipeline->setYoloModelPath(path);
        }
        emit yoloModelPathChanged(path);
    }
}

void VideoController::start() {
    if (!m_pipeline) {
        m_pipeline = new VideoPipeline(m_port);
        if (!m_pipeline->initialize()) {
            qWarning() << "Failed to initialize pipeline";
            delete m_pipeline;
            m_pipeline = nullptr;
            return;
        }

        // Подключаем сигналы после создания pipeline
        connect(m_pipeline, &VideoPipeline::newFrame, this, &VideoController::newFrame, Qt::DirectConnection);
        connect(m_pipeline, &VideoPipeline::newObjects, this, &VideoController::newObjects, Qt::DirectConnection);

        if (!m_yoloModelPath.isEmpty()) {
            m_pipeline->setYoloModelPath(m_yoloModelPath);
        }
        m_pipeline->setYoloEnabled(m_yoloEnabled);
    }

    m_pipeline->start();
    m_isRunning = true;
    emit isRunningChanged(true);
}

void VideoController::stop()
{
    resetPipeline();
}

void VideoController::resetPipeline()
{
    if (m_pipeline) {
        m_pipeline->stop();
        delete m_pipeline;
        m_pipeline = nullptr;
        m_isRunning = false;
        emit isRunningChanged(false);
    }
}

