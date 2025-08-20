#include "videocontroller.h"
#include <QDebug>

VideoController::VideoController(QObject *parent) : QObject(parent)
{
    deviceClient = new ModbusCommandsClient(this);
    connect(deviceClient, &ModbusClient::connectionStateChanged,
            this, &VideoController::onModbusConnectionStateChanged);
    connect(deviceClient, &ModbusClient::errorOccurred,
            this, &VideoController::onModbusError);
}

VideoController::~VideoController()
{
    stop();
    delete m_pipeline;
}

bool VideoController::isRunning() const { return m_isRunning; }
int VideoController::port() const { return m_port; }
bool VideoController::yoloEnabled() const { return m_yoloEnabled; }
QString VideoController::yoloModelPath() const { return m_yoloModelPath; }
QVariantList VideoController::objects() const {
    QVariantList list;
    for (const auto &obj : m_objects) {
        QVariantMap map;
        map["rect"] = QVariant::fromValue(obj.first);
        map["label"] = obj.second;
        list.append(map);
    }
    return list;
}

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
        emit objectsChanged(objects());
    }
}

int VideoController::fps() const
{
    return m_fps;
}

void VideoController::onFpsChanged(int fps)
{
    if (m_fps != fps) {
        m_fps = fps;
        emit fpsChanged(fps);
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

        connect(m_pipeline, &VideoPipeline::newFrame, this, &VideoController::newFrame);
        connect(m_pipeline, &VideoPipeline::newObjects, this, [this](const QList<QPair<QRect, QString>>& objects) {
            m_objects = objects;
            emit objectsChanged(this->objects());
        });
        connect(m_pipeline, &VideoPipeline::fpsChanged, this, &VideoController::onFpsChanged);

        if (!m_yoloModelPath.isEmpty()) {
            m_pipeline->setYoloModelPath(m_yoloModelPath);
        }
        m_pipeline->setYoloEnabled(m_yoloEnabled);
    }

    m_pipeline->start();
    m_isRunning = true;
    emit isRunningChanged(true);
}

void VideoController::stop() {
    resetPipeline();
}

void VideoController::resetPipeline() {
    if (m_pipeline) {
        m_pipeline->stop();
        disconnect(m_pipeline, nullptr, this, nullptr);
        delete m_pipeline;
        m_pipeline = nullptr;
        m_isRunning = false;
        m_objects.clear();
        emit isRunningChanged(false);
        emit objectsChanged(objects());
    }
}

bool VideoController::modbusConnected() const {
    return deviceClient ? deviceClient->isConnected() : false;
}

QString VideoController::modbusAddress() const {
    return m_modbusAddress;
}

void VideoController::setModbusAddress(const QString& address) {
    if (m_modbusAddress != address) {
        m_modbusAddress = address;
        qDebug() << "Setting Modbus address to:" << address;
        deviceClient->setupDevice(address);
        deviceClient->setConnectionSettings(1000, 3);
        emit modbusAddressChanged(address);
    }
}

void VideoController::connectModbus() {
    if (!m_modbusAddress.isEmpty()) {
        qDebug() << "Attempting to connect Modbus to:" << m_modbusAddress;
        deviceClient->connectDevice();
    } else {
        qWarning() << "Modbus address is empty!";
        emit modbusErrorOccurred("Modbus address is empty");
    }
}

void VideoController::disconnectModbus() {
    deviceClient->disconnectDevice();
}

void VideoController::onModbusConnectionStateChanged(bool connected) {
    emit modbusConnectedChanged(connected);
}

void VideoController::onModbusError(const QString& error) {
    emit modbusErrorOccurred(error);
}

void VideoController::sendRebootCommand()
{
    if (deviceClient && deviceClient->isConnected()) {
        deviceClient->sendRebootCommand(1); // serverAddress = 1
    } else {
        emit modbusErrorOccurred("Modbus not connected");
    }
}
