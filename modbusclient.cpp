#include "modbusclient.h"
#include <QModbusTcpClient>
#include <QUrl>

ModbusClient::ModbusClient(QObject *parent) : QObject(parent)
{
}

ModbusClient::~ModbusClient()
{
    if (modbusDevice) {
        modbusDevice->disconnectDevice();
        delete modbusDevice;
    }
}

void ModbusClient::setupDevice(const QString &connectionParam)
{
    if (modbusDevice) {
        modbusDevice->disconnectDevice();
        delete modbusDevice;
        modbusDevice = nullptr;
    }

    modbusDevice = new QModbusTcpClient(this);
    if (!modbusDevice) { // Правильная проверка
        emit errorOccurred("Failed to create Modbus device");
        return;
    }

    const QUrl url = QUrl::fromUserInput(connectionParam);
    if (!url.isValid()) {
        emit errorOccurred("Invalid URL format");
        return;
    }

    modbusDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, QVariant(url.port()));
    modbusDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, QVariant(url.host()));

    connect(modbusDevice, &QModbusClient::errorOccurred, this, [this](QModbusDevice::Error) {
        emit errorOccurred(modbusDevice->errorString());
    });
    connect(modbusDevice, &QModbusClient::stateChanged, this, &ModbusClient::onModbusStateChanged);
}

void ModbusClient::setConnectionSettings(int responseTime, int numberOfRetries)
{
    if (modbusDevice) {
        modbusDevice->setTimeout(responseTime);
        modbusDevice->setNumberOfRetries(numberOfRetries);
    }
}

void ModbusClient::connectDevice()
{
    if (modbusDevice && modbusDevice->state() != QModbusDevice::ConnectedState) {
        qDebug() << "Connecting to Modbus at:" << modbusDevice->connectionParameter(QModbusDevice::NetworkAddressParameter).toString()
        << "port:" << modbusDevice->connectionParameter(QModbusDevice::NetworkPortParameter).toInt();

        if (!modbusDevice->connectDevice()) {
            QString error = tr("Connect failed: %1").arg(modbusDevice->errorString());
            qWarning() << error;
            emit errorOccurred(error);
        } else {
            qDebug() << "Connection attempt started";
        }
    }
}

void ModbusClient::disconnectDevice()
{
    if (modbusDevice && modbusDevice->state() == QModbusDevice::ConnectedState) {
        modbusDevice->disconnectDevice();
    }
}

bool ModbusClient::isConnected() const
{
    return modbusDevice && modbusDevice->state() == QModbusDevice::ConnectedState;
}

QModbusDataUnit ModbusClient::createReadRequest(QModbusDataUnit::RegisterType table,
                                                int startAddress, quint16 numberOfEntries) const
{
    return QModbusDataUnit(table, startAddress, numberOfEntries);
}

QModbusDataUnit ModbusClient::createWriteRequest(QModbusDataUnit::RegisterType table,
                                                 int startAddress, quint16 numberOfEntries) const
{
    return QModbusDataUnit(table, startAddress, numberOfEntries);
}

void ModbusClient::sendReadRequest(const QModbusDataUnit &unit, int serverAddress)
{
    if (!modbusDevice) return;

    if (auto *reply = modbusDevice->sendReadRequest(unit, serverAddress)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &ModbusClient::onReadReady);
        else
            delete reply;
    } else {
        emit errorOccurred(tr("Read error: %1").arg(modbusDevice->errorString()));
    }
}

void ModbusClient::sendWriteRequest(const QModbusDataUnit &unit, int serverAddress)
{
    if (!modbusDevice) return;

    if (auto *reply = modbusDevice->sendWriteRequest(unit, serverAddress)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &ModbusClient::onWriteFinished);
        else
            delete reply;
    } else {
        emit errorOccurred(tr("Write error: %1").arg(modbusDevice->errorString()));
    }
}

void ModbusClient::sendReadWriteRequest(const QModbusDataUnit &readUnit,
                                        const QModbusDataUnit &writeUnit, int serverAddress)
{
    if (!modbusDevice) return;

    if (auto *reply = modbusDevice->sendReadWriteRequest(readUnit, writeUnit, serverAddress)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &ModbusClient::onReadReady);
        else
            delete reply;
    } else {
        emit errorOccurred(tr("Read-write error: %1").arg(modbusDevice->errorString()));
    }
}

void ModbusClient::onModbusStateChanged(int state)
{
    qDebug() << "Modbus state changed to:" << state;
    emit connectionStateChanged(state == QModbusDevice::ConnectedState);
}

void ModbusClient::onReadReady()
{
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) return;

    if (reply->error() == QModbusDevice::NoError) {
        emit readReady(reply->result());
    } else {
        QString errorMsg;
        if (reply->error() == QModbusDevice::ProtocolError) {
            errorMsg = tr("Read response error: %1 (Modbus exception: 0x%2)")
            .arg(reply->errorString())
                .arg(reply->rawResult().exceptionCode(), -1, 16);
        } else {
            errorMsg = tr("Read response error: %1 (code: 0x%2)")
            .arg(reply->errorString())
                .arg(reply->error(), -1, 16);
        }
        emit errorOccurred(errorMsg);
    }

    reply->deleteLater();
}

void ModbusClient::onWriteFinished()
{
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) return;

    if (reply->error() == QModbusDevice::NoError) {
        emit writeFinished();
    } else {
        QString errorMsg;
        if (reply->error() == QModbusDevice::ProtocolError) {
            errorMsg = tr("Write response error: %1 (Modbus exception: 0x%2)")
            .arg(reply->errorString())
                .arg(reply->rawResult().exceptionCode(), -1, 16);
        } else {
            errorMsg = tr("Write response error: %1 (code: 0x%2)")
            .arg(reply->errorString())
                .arg(reply->error(), -1, 16);
        }
        emit errorOccurred(errorMsg);
    }

    reply->deleteLater();
}
