#include "videorenderer.h"
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLPaintDevice>
#include <QPainter>

VideoRenderer::VideoRenderer(QQuickItem *parent) : QQuickFramebufferObject(parent)
{
    setMirrorVertically(false);
}

QQuickFramebufferObject::Renderer *VideoRenderer::createRenderer() const
{
    return new VideoRendererImpl(const_cast<VideoRenderer*>(this));
}

bool VideoRenderer::showObjects() const { return m_showObjects; }

void VideoRenderer::setShowObjects(bool show) {
    if (m_showObjects != show) {
        m_showObjects = show;
        emit showObjectsChanged(show);
    }
}

void VideoRenderer::updateFrame(const QImage &frame)
{
    QMutexLocker locker(&m_frameMutex);
    m_frame = frame;
    update();
}

void VideoRenderer::updateObjects(const QList<QRect> &objects) {
    QMutexLocker locker(&m_frameMutex);
    m_objects = objects;
    update();
}

VideoRendererImpl::VideoRendererImpl(VideoRenderer *item) : m_item(item)
{
    initializeOpenGLFunctions();
    glGenTextures(1, &m_texture);
    initShader();
    initBuffers();
    m_paintDevice = new QOpenGLPaintDevice();
}

VideoRendererImpl::~VideoRendererImpl()
{
    glDeleteTextures(1, &m_texture);
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
    delete m_shader;
    delete m_paintDevice;
}

void VideoRendererImpl::initShader() {
    m_shader = new QOpenGLShaderProgram();
    m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                      "attribute vec2 position;"
                                      "attribute vec2 texCoord;"
                                      "varying vec2 vTexCoord;"
                                      "void main() {"
                                      "    gl_Position = vec4(position, 0.0, 1.0);"
                                      "    vTexCoord = texCoord;"
                                      "}");
    m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                      "varying vec2 vTexCoord;"
                                      "uniform sampler2D texture;"
                                      "void main() {"
                                      "    gl_FragColor = texture2D(texture, vTexCoord);"
                                      "}");
    m_shader->link();
}

void VideoRendererImpl::initBuffers() {
    GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        1.0f,  1.0f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

    glBindVertexArray(0);
}

void VideoRendererImpl::render() {
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    QMutexLocker locker(&m_frameMutex);
    if (m_frame.isNull()) return;

    QSize viewportSize = framebufferObject()->size();
    glViewport(0, 0, viewportSize.width(), viewportSize.height());

    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_frame.width(), m_frame.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, m_frame.bits());

    m_shader->bind();
    glBindVertexArray(m_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    m_shader->setUniformValue("texture", 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (m_showObjects && !m_objects.isEmpty() && !m_frame.isNull()) {
        m_paintDevice->setSize(framebufferObject()->size());
        QPainter painter(m_paintDevice);
        painter.setPen(QPen(Qt::red, 2));

        // Правильное масштабирование прямоугольников
        float scaleX = (float)viewportSize.width() / m_frame.width();
        float scaleY = (float)viewportSize.height() / m_frame.height();

        for (const QRect& rect : m_objects) {
            QRect scaledRect(
                rect.x() * scaleX,
                rect.y() * scaleY,
                rect.width() * scaleX,
                rect.height() * scaleY
                );
            painter.drawRect(scaledRect);
        }
    }

    glBindVertexArray(0);
    m_shader->release();
}

QOpenGLFramebufferObject *VideoRendererImpl::createFramebufferObject(const QSize &size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    return new QOpenGLFramebufferObject(size, format);
}

void VideoRendererImpl::synchronize(QQuickFramebufferObject *item) {
    VideoRenderer *renderer = static_cast<VideoRenderer*>(item);
    QMutexLocker locker(&renderer->m_frameMutex);
    m_frame = renderer->m_frame;
    m_objects = renderer->m_objects;
    m_showObjects = renderer->m_showObjects;
}
