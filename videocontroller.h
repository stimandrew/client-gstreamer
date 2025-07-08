#pragma once

#include <QObject>
#include "videopipeline.h"

class VideoController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)

public:
    explicit VideoController(QObject *parent = nullptr);
    ~VideoController();

    bool isRunning() const;
    int port() const;
    void setPort(int port);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

signals:
    void isRunningChanged(bool isRunning);
    void portChanged(int port);
    void newFrame(const QImage &frame);

private:
    VideoPipeline *m_pipeline = nullptr;
    int m_port = 0;
    bool m_isRunning = false;
};
