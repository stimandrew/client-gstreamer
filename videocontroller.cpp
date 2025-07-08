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

void VideoController::setPort(int port)
{
    if (m_port != port) {
        resetPipeline();
        m_port = port;
        emit portChanged(port);
    }
}

void VideoController::start()
{
    if (!m_pipeline) {
        m_pipeline = new VideoPipeline(m_port);
        if (!m_pipeline->initialize()) {
            qWarning() << "Failed to initialize pipeline for port" << m_port;
            delete m_pipeline;
            m_pipeline = nullptr;
            return;
        }
        connect(m_pipeline, &VideoPipeline::newFrame, this, &VideoController::newFrame);
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

