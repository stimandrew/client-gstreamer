#pragma once

#include <QObject>
#include <QModbusDataUnit>
#include <QModbusClient>
#include <QVariant>

class ModbusClient : public QObject
{
    Q_OBJECT

public:
    explicit ModbusClient(QObject *parent = nullptr);
    ~ModbusClient();

    void setupDevice(const QString &connectionParam);
    void setConnectionSettings(int responseTime, int numberOfRetries);
    void connectDevice();
    void disconnectDevice();
    bool isConnected() const;

    QModbusDataUnit createReadRequest(QModbusDataUnit::RegisterType table,
                                      int startAddress, quint16 numberOfEntries) const;
    QModbusDataUnit createWriteRequest(QModbusDataUnit::RegisterType table,
                                       int startAddress, quint16 numberOfEntries) const;

    void sendReadRequest(const QModbusDataUnit &unit, int serverAddress);
    void sendWriteRequest(const QModbusDataUnit &unit, int serverAddress);
    void sendReadWriteRequest(const QModbusDataUnit &readUnit,
                              const QModbusDataUnit &writeUnit, int serverAddress);

    void sendRebootCommand(int serverAddress);

signals:
    void connectionStateChanged(bool connected);
    void readReady(const QModbusDataUnit &unit);
    void writeFinished();
    void errorOccurred(const QString &error);

private slots:
    void onModbusStateChanged(int state);
    void onReadReady();
    void onWriteFinished();

private:
    QModbusClient *modbusDevice = nullptr;
};

