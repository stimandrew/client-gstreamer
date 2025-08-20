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
