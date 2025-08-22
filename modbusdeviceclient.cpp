#include "modbusdeviceclient.h"
#include <QDebug>

// Конструктор: инициализация объекта ModbusDeviceClient
ModbusDeviceClient::ModbusDeviceClient(QObject *parent) : ModbusClient(parent)
{
}

// Отправка команды перезагрузки на указанный сервер Modbus
void ModbusDeviceClient::sendRebootCommand(int serverAddress)
{
    if (!modbusDevice) {
        qWarning() << "Modbus device is null!";
        emit errorOccurred("Modbus device not initialized");
        return;
    }

    if (!isConnected()) {
        qWarning() << "Modbus device not connected!";
        emit errorOccurred("Modbus device not connected");
        return;
    }

    qDebug() << "Sending reboot command to server address:" << serverAddress;

    // Создание запроса на запись в holding register 100
    QModbusDataUnit writeUnit = createWriteRequest(QModbusDataUnit::HoldingRegisters, 100, 1);
    writeUnit.setValue(0, 1); // Установка значения 1 для команды перезагрузки

    qDebug() << "Writing value 1 to holding register 100";

    // Отправка запроса на запись
    sendWriteRequest(writeUnit, serverAddress);

    qDebug() << "Reboot command sent successfully";
}

void ModbusDeviceClient::writeCoil(int address, bool value, int serverAddress)
{
    if (!modbusDevice) {
        qWarning() << "Modbus device is null!";
        emit errorOccurred("Modbus device not initialized");
        return;
    }

    if (!isConnected()) {
        qWarning() << "Modbus device not connected!";
        emit errorOccurred("Modbus device not connected");
        return;
    }

    qDebug() << "Writing coil" << address << "value:" << value;

    // Создание запроса на запись в coil
    QModbusDataUnit writeUnit = createWriteRequest(QModbusDataUnit::Coils, address, 1);
    writeUnit.setValue(0, value ? 0xFF00 : 0x0000); // Modbus спецификация: 0xFF00 = ON, 0x0000 = OFF

    // Отправка запроса на запись
    sendWriteRequest(writeUnit, serverAddress);

    qDebug() << "Coil write command sent successfully";
}

void ModbusDeviceClient::readCameraStates(int serverAddress)
{
    if (!modbusDevice) {
        qWarning() << "Modbus device is null!";
        emit errorOccurred("Modbus device not initialized");
        return;
    }

    if (!isConnected()) {
        qWarning() << "Modbus device not connected!";
        emit errorOccurred("Modbus device not connected");
        return;
    }

    QModbusDataUnit readUnit = createReadRequest(QModbusDataUnit::Coils, 0, 10);

    if (auto *reply = modbusDevice->sendReadRequest(readUnit, serverAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [this, reply]() {
                if (reply->error() == QModbusDevice::NoError) {
                    const QModbusDataUnit result = reply->result();
                    for (int i = 0; i < result.valueCount(); i++) {
                        bool isActive = result.value(i) != 0;
                        emit cameraStateChanged(i, isActive);
                    }
                } else {
                    emit errorOccurred(QString("Failed to read camera states: %1").arg(reply->errorString()));
                }
                reply->deleteLater();
            });
        } else {
            delete reply;
            emit errorOccurred("Failed to send read request");
        }
    } else {
        emit errorOccurred(QString("Failed to create read request: %1").arg(modbusDevice->errorString()));
    }
}

