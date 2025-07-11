#pragma once

#include <QObject>
#include "videopipeline.h"

class VideoController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(bool yoloEnabled READ yoloEnabled WRITE setYoloEnabled NOTIFY yoloEnabledChanged)
    Q_PROPERTY(QString yoloModelPath READ yoloModelPath WRITE setYoloModelPath NOTIFY yoloModelPathChanged)

public:
    explicit VideoController(QObject *parent = nullptr);
    ~VideoController();

    bool isRunning() const;
    int port() const;
    bool yoloEnabled() const;
    QString yoloModelPath() const;

    Q_INVOKABLE void setPort(int port);
    Q_INVOKABLE void setYoloEnabled(bool enabled);
    Q_INVOKABLE void setYoloModelPath(const QString& path);
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

signals:
    void isRunningChanged(bool isRunning);
    void portChanged(int port);
    void yoloEnabledChanged(bool enabled);
    void yoloModelPathChanged(const QString& path);
    void newFrame(const QImage &frame);
    void newObjects(const QList<QRect> &objects);

private:
    VideoPipeline *m_pipeline = nullptr;
    int m_port = 0;
    bool m_isRunning = false;
    bool m_yoloEnabled = false;
    QString m_yoloModelPath;
    void resetPipeline();
};
