#include "videorenderer.h"
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>

VideoRenderer::VideoRenderer(QQuickItem *parent) : QQuickFramebufferObject(parent)
{
    setMirrorVertically(false);
}

QQuickFramebufferObject::Renderer *VideoRenderer::createRenderer() const
{
    return new VideoRendererImpl(const_cast<VideoRenderer*>(this));
}

void VideoRenderer::updateFrame(const QImage &frame)
{
    QMutexLocker locker(&m_frameMutex);
    m_frame = frame;
    update();
}

VideoRendererImpl::VideoRendererImpl(VideoRenderer *item) : m_item(item)
{
    initializeOpenGLFunctions();
    glGenTextures(1, &m_texture);
    initShader();
    initBuffers();
}

VideoRendererImpl::~VideoRendererImpl()
{
    glDeleteTextures(1, &m_texture);
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
    delete m_shader;
}

void VideoRendererImpl::initShader()
{
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

void VideoRendererImpl::initBuffers()
{
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

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

    glBindVertexArray(0);
}

void VideoRendererImpl::render()
{
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    QMutexLocker locker(&m_frameMutex);
    if (m_frame.isNull()) return;

    glViewport(0, 0, m_frame.width(), m_frame.height());

    // Обновляем текстуру
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_frame.width(), m_frame.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, m_frame.bits());

    // Рисуем
    m_shader->bind();
    glBindVertexArray(m_vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    m_shader->setUniformValue("texture", 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindVertexArray(0);
    m_shader->release();
}

QOpenGLFramebufferObject *VideoRendererImpl::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}

void VideoRendererImpl::synchronize(QQuickFramebufferObject *item)
{
    VideoRenderer *renderer = static_cast<VideoRenderer*>(item);
    QMutexLocker locker(&renderer->m_frameMutex);
    m_frame = renderer->m_frame;
}
