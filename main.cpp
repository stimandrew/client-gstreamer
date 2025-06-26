#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QRunnable>
#include <gst/gst.h>

class SetPlaying : public QRunnable
{
public:
    SetPlaying(GstElement *);
    ~SetPlaying();

    void run ();

private:
    GstElement * pipeline_;
};

SetPlaying::SetPlaying (GstElement * pipeline)
{
    this->pipeline_ = pipeline ? static_cast<GstElement *> (gst_object_ref (pipeline)) : NULL;
}

SetPlaying::~SetPlaying ()
{
    if (this->pipeline_)
        gst_object_unref (this->pipeline_);
}

void
SetPlaying::run ()
{
    if (this->pipeline_)
        gst_element_set_state (this->pipeline_, GST_STATE_PLAYING);
}

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "wayland");
    qputenv("GST_DEBUG", "4");
    qputenv("GST_DEBUG_NO_COLOR", "1");
    int ret;

    gst_init (&argc, &argv);

    {
        QGuiApplication app(argc, argv);

        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

        GstElement *pipeline = gst_pipeline_new (NULL);
        // 1. Добавляем caps для udpsrc
        GstElement *src = gst_element_factory_make ("udpsrc", NULL);
        g_object_set (src,
                     "port", 5000,
                     "caps", gst_caps_from_string("application/x-rtp,media=video,encoding-name=H264"),
                     NULL);

        // 2. Добавляем rtpjitterbuffer для обработки сетевых задержек
        GstElement *jitterbuffer = gst_element_factory_make ("rtpjitterbuffer", NULL);
        g_object_set (jitterbuffer, "latency", 200, NULL); // 200 мс задержки

        // 3. Остальные элементы
        GstElement *rtpdepay = gst_element_factory_make ("rtph264depay", NULL);
        GstElement *parse = gst_element_factory_make ("h264parse", NULL);
        GstElement *decoder = gst_element_factory_make ("mppvideodec", NULL);
        GstElement *queue = gst_element_factory_make("queue", NULL); // Добавлено!
        g_object_set(queue, "max-size-buffers", 3, NULL); // Лимит буфера
        GstElement *glupload = gst_element_factory_make ("glupload", NULL);
        GstElement *convert = gst_element_factory_make("glcolorconvert", NULL);
        GstElement *sink = gst_element_factory_make ("qml6glsink", NULL);

        g_assert(src &&
                 jitterbuffer &&
                 rtpdepay &&
                 parse &&
                 decoder &&
                 queue &&
                 glupload &&
                 convert &&
                 sink);
        g_object_set(sink, "sync", FALSE, NULL); // Важно!

        gst_bin_add_many (GST_BIN (pipeline),
                         src,
                         jitterbuffer,
                         rtpdepay,
                         parse,
                         decoder,
                         queue,    // Буфер после декодера
                         glupload,
                         convert,
                         sink,
                         NULL);

        // 5. Соединяем элементы
        if (!gst_element_link_many (
                src,
                jitterbuffer,
                rtpdepay,
                parse,
                decoder,
                queue,    // Буферизация здесь
                glupload,
                convert,
                sink,
                NULL))
        {
            g_printerr ("Ошибка соединения элементов!\n");
            return -1;
        }

        QQmlApplicationEngine engine;
        engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

        QQuickItem *videoItem;
        QQuickWindow *rootObject;

        /* find and set the videoItem on the sink */
        rootObject = static_cast<QQuickWindow *> (engine.rootObjects().first());
        videoItem = rootObject->findChild<QQuickItem *> ("videoItem");
        g_assert (videoItem);
        g_object_set(sink, "widget", videoItem, NULL);

        rootObject->scheduleRenderJob (new SetPlaying (pipeline),
                                      QQuickWindow::BeforeSynchronizingStage);

        ret = app.exec();

        gst_element_set_state (pipeline, GST_STATE_NULL);
        gst_object_unref (pipeline);
    }

    gst_deinit ();

    return ret;
}

