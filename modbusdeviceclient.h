#pragma once
#include "modbusclient.h"

// Класс-наследник для специфических команд устройства Modbus
class ModbusDeviceClient : public ModbusClient
{
    Q_OBJECT
public:
    // Конструктор класса ModbusDeviceClient
    explicit ModbusDeviceClient(QObject *parent = nullptr);

    // Отправка команды перезагрузки на указанный сервер Modbus
    void sendRebootCommand(int serverAddress);
};
