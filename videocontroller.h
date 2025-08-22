#pragma once

#include <QObject>
#include <QVariant>
#include <QMap>
#include <QTimer>
#include "videopipeline.h"
#include "modbusdeviceclient.h"

class VideoController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(bool yoloEnabled READ yoloEnabled WRITE setYoloEnabled NOTIFY yoloEnabledChanged)
    Q_PROPERTY(QString yoloModelPath READ yoloModelPath WRITE setYoloModelPath NOTIFY yoloModelPathChanged)
    Q_PROPERTY(QVariantList objects READ objects NOTIFY objectsChanged)
    Q_PROPERTY(int fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(bool modbusConnected READ modbusConnected NOTIFY modbusConnectedChanged)
    Q_PROPERTY(QString modbusAddress READ modbusAddress WRITE setModbusAddress NOTIFY modbusAddressChanged)
    Q_PROPERTY(bool camera1Active READ camera1Active NOTIFY cameraStatesChanged)
    Q_PROPERTY(bool camera2Active READ camera2Active NOTIFY cameraStatesChanged)
    Q_PROPERTY(bool camera3Active READ camera3Active NOTIFY cameraStatesChanged)

public:
    explicit VideoController(QObject *parent = nullptr);
    ~VideoController();

    bool isRunning() const;
    int port() const;
    int fps() const;
    bool yoloEnabled() const;
    QString yoloModelPath() const;
    QVariantList objects() const;

    bool modbusConnected() const;
    QString modbusAddress() const;
    bool camera1Active() const;
    bool camera2Active() const;
    bool camera3Active() const;

    Q_INVOKABLE void setPort(int port);
    Q_INVOKABLE void setYoloEnabled(bool enabled);
    Q_INVOKABLE void setYoloModelPath(const QString& path);
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

    Q_INVOKABLE void setModbusAddress(const QString& address);
    Q_INVOKABLE void connectModbus();
    Q_INVOKABLE void disconnectModbus();

    Q_INVOKABLE void sendRebootCommand();
    Q_INVOKABLE void writeCoil(int address, bool value);
    Q_INVOKABLE void readCameraStates();

signals:
    void isRunningChanged(bool isRunning);
    void portChanged(int port);
    void yoloEnabledChanged(bool enabled);
    void yoloModelPathChanged(const QString& path);
    void objectsChanged(const QVariantList& objects);
    void newFrame(const QImage &frame);
    void newObjects(const QList<QRect> &objects);
    void fpsChanged(int fps);
    void modbusConnectedChanged(bool connected);
    void modbusAddressChanged(const QString& address);
    void modbusErrorOccurred(const QString& error);
    void cameraStatesChanged();

private slots:
    void onFpsChanged(int fps);
    void onModbusConnectionStateChanged(bool connected);
    void onModbusError(const QString& error);
    void onCameraStateChanged(int cameraIndex, bool isActive);

private:
    VideoPipeline *m_pipeline = nullptr;
    int m_port = 0;
    bool m_isRunning = false;
    bool m_yoloEnabled = false;
    QString m_yoloModelPath;
    QList<QPair<QRect, QString>> m_objects;
    void resetPipeline();
    int m_fps = 0;

    ModbusDeviceClient *deviceClient = nullptr;
    QString m_modbusAddress;
    QMap<int, bool> m_cameraStates;
    QTimer *m_updateTimer = nullptr;
};
