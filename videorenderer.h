#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QMutex>
#include <QImage>

class VideoRenderer : public QQuickFramebufferObject
{
    Q_OBJECT
public:
    VideoRenderer(QQuickItem *parent = nullptr);
    Renderer *createRenderer() const override;

public slots:
    void updateFrame(const QImage &frame);

protected:
    friend class VideoRendererImpl;
    QImage m_frame;
    mutable QMutex m_frameMutex;
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
    QMutex m_frameMutex;
    GLuint m_texture = 0;
    GLuint m_vbo = 0;
    GLuint m_vao = 0;
    QOpenGLShaderProgram *m_shader = nullptr;
};
