#pragma once
#include "modbusclient.h"

// Класс-наследник для специфических команд устройства Modbus
class ModbusCommandsClient : public ModbusClient
{
    Q_OBJECT

public:
    // Конструктор класса ModbusDeviceClient
    explicit ModbusCommandsClient(QObject *parent = nullptr);

    // Отправка команды перезагрузки на указанный сервер Modbus
    void sendRebootCommand(int serverAddress);
};
