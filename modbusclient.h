#pragma once

#include <QObject>
#include <QModbusDataUnit>
#include <QModbusClient>
#include <QVariant>

class ModbusClient : public QObject
{
    Q_OBJECT

public:
    // Конструктор класса ModbusClient
    explicit ModbusClient(QObject *parent = nullptr);

    // Виртуальный деструктор для корректного наследования
    virtual ~ModbusClient();

    // Настройка Modbus устройства с указанными параметрами подключения
    void setupDevice(const QString &connectionParam);

    // Установка параметров подключения: время ответа и количество повторов
    void setConnectionSettings(int responseTime, int numberOfRetries);

    // Подключение к Modbus устройству
    void connectDevice();

    // Отключение от Modbus устройства
    void disconnectDevice();

    // Проверка состояния подключения
    bool isConnected() const;

    // Создание запроса на чтение данных
    QModbusDataUnit createReadRequest(QModbusDataUnit::RegisterType table,
                                      int startAddress, quint16 numberOfEntries) const;

    // Создание запроса на запись данных
    QModbusDataUnit createWriteRequest(QModbusDataUnit::RegisterType table,
                                       int startAddress, quint16 numberOfEntries) const;

    // Отправка запроса на чтение данных
    void sendReadRequest(const QModbusDataUnit &unit, int serverAddress);

    // Отправка запроса на запись данных
    void sendWriteRequest(const QModbusDataUnit &unit, int serverAddress);

    // Отправка комбинированного запроса на чтение-запись
    void sendReadWriteRequest(const QModbusDataUnit &readUnit,
                              const QModbusDataUnit &writeUnit, int serverAddress);

signals:
    // Сигнал изменения состояния подключения
    void connectionStateChanged(bool connected);

    // Сигнал готовности данных после чтения
    void readReady(const QModbusDataUnit &unit);

    // Сигнал завершения операции записи
    void writeFinished();

    // Сигнал возникновения ошибки
    void errorOccurred(const QString &error);

protected:
    // Защищенный указатель на устройство для доступа из классов-наследников
    QModbusClient *modbusDevice = nullptr;

private slots:
    // Обработчик изменения состояния Modbus устройства
    void onModbusStateChanged(int state);

    // Обработчик завершения операции чтения
    void onReadReady();

    // Обработчик завершения операции записи
    void onWriteFinished();
};
