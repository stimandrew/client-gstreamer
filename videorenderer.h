#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QMutex>
#include <QImage>
#include <QOpenGLPaintDevice>

class VideoRenderer : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(bool showObjects READ showObjects WRITE setShowObjects NOTIFY showObjectsChanged)
public:
    VideoRenderer(QQuickItem *parent = nullptr);
    Renderer *createRenderer() const override;

    bool showObjects() const;
    void setShowObjects(bool show);

public slots:
    void updateFrame(const QImage &frame);
    void updateObjects(const QList<QRect> &objects);

signals:
    void showObjectsChanged(bool show);

protected:
    friend class VideoRendererImpl;
    QImage m_frame;
    QList<QRect> m_objects;
    mutable QMutex m_frameMutex;
    bool m_showObjects = false;
};

class VideoRendererImpl : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions
{
public:
    VideoRendererImpl(VideoRenderer *item);
    ~VideoRendererImpl() override;
    void render() override;
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;
    void synchronize(QQuickFramebufferObject *item) override;

private:
    void initShader();
    void initBuffers();

    VideoRenderer *m_item;
    QImage m_frame;
    QList<QRect> m_objects;
    QMutex m_frameMutex;
    GLuint m_texture = 0;
    GLuint m_vbo = 0;
    GLuint m_vao = 0;
    QOpenGLShaderProgram *m_shader = nullptr;
    QOpenGLPaintDevice *m_paintDevice = nullptr;
    bool m_showObjects = false;
};
