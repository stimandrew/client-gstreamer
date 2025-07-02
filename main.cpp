#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QRunnable>
#include <QFile>
#include <gst/gst.h>
#include <gst/gstelementfactory.h>
#include <QDirIterator>
#include <QTimer>

typedef struct {
    GstPad *sinkpad1;
    GstPad *sinkpad2;
} MixerPads;

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

static void
on_mixer_scene_initialized (GstElement * mixer, gpointer user_data)
{
    g_print("Mixer scene initialized\n");
    MixerPads *mixer_pads = (MixerPads *)user_data;
    QQuickItem *rootObject;
    g_object_get (mixer, "root-item", &rootObject, NULL);

    QQuickItem *videoItem0 = rootObject->findChild<QQuickItem *>("inputVideoItem0");
    QQuickItem *videoItem1 = rootObject->findChild<QQuickItem *>("inputVideoItem1");

    if (!videoItem0 || !videoItem1) {
        g_printerr("Error: Could not find QML video items\n");
        return;
    }

    // Связываем с сохраненными площадками
    g_object_set(mixer_pads->sinkpad1, "widget", videoItem0, NULL);
    g_object_set(mixer_pads->sinkpad2, "widget", videoItem1, NULL);
    g_print("VideoItem0 size: %f x %f\n", videoItem0->width(), videoItem0->height());
    g_print("VideoItem1 size: %f x %f\n", videoItem1->width(), videoItem1->height());

    gst_object_unref(mixer_pads->sinkpad1);
    gst_object_unref(mixer_pads->sinkpad2);

}

static GstPadProbeReturn caps_logger_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    if (GST_EVENT_TYPE(GST_PAD_PROBE_INFO_EVENT(info)) == GST_EVENT_CAPS) {
        GstCaps *caps;
        gst_event_parse_caps(GST_PAD_PROBE_INFO_EVENT(info), &caps);
        g_printerr("\n--- CAPS LOGGER [%s] ---\n", (gchar*)user_data);
        g_printerr("%s\n", gst_caps_to_string(caps));
        g_printerr("-------------------------\n");
    }
    return GST_PAD_PROBE_OK;
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

        // Создаем структуру для хранения площадок
        MixerPads *mixer_pads = g_new0(MixerPads, 1);

        GstElement *pipeline = gst_pipeline_new (NULL);

        GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
        gst_bus_add_watch(bus, [](GstBus *bus, GstMessage *msg, gpointer user_data) -> gboolean {
            GstElement *pipeline = static_cast<GstElement*>(user_data);
            switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR: {
                GError *err = NULL;
                gchar *debug = NULL;
                gst_message_parse_error(msg, &err, &debug);
                g_printerr("\n--- GST ERROR ---\n");
                g_printerr("Source: %s\n", GST_OBJECT_NAME(msg->src));
                g_printerr("Error: %s\n", err->message);
                g_printerr("Debug: %s\n", debug ? debug : "none");
                g_printerr("----------------\n");
                g_error_free(err);
                g_free(debug);
                break;
            }
            case GST_MESSAGE_WARNING: {
                GError *err = NULL;
                gchar *debug = NULL;
                gst_message_parse_warning(msg, &err, &debug);
                g_printerr("\n--- GST WARNING ---\n");
                g_printerr("Source: %s\n", GST_OBJECT_NAME(msg->src));
                g_printerr("Warning: %s\n", err->message);
                g_printerr("Debug: %s\n", debug ? debug : "none");
                g_printerr("------------------\n");
                g_error_free(err);
                g_free(debug);
                break;
            }
            case GST_MESSAGE_STATE_CHANGED: {
                if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
                    GstState old_state, new_state, pending;
                    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending);
                    g_print("\n--- STATE CHANGE ---\n");
                    g_print("%s -> %s\n",
                            gst_element_state_get_name(old_state),
                            gst_element_state_get_name(new_state));
                    g_print("-------------------\n");
                }
                break;
            }
            case GST_MESSAGE_EOS:
                g_print("\n--- EOS RECEIVED ---\n");
                break;
            default:
                break;
            }
            return TRUE;
        }, pipeline); // Передаём pipeline как user_data

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
        GstElement *tee = gst_element_factory_make("tee", NULL);


        GstElement *queue1 = gst_element_factory_make("queue", NULL);
        g_object_set(queue1, "max-size-buffers", 10, NULL); // Лимит буфера
        GstElement *decoder1 = gst_element_factory_make ("mppvideodec", NULL);
        g_object_set(decoder1, "format", 11, NULL);
        GstElement *queue11 = gst_element_factory_make("queue", NULL); // Добавлено!
        g_object_set(queue11, "max-size-buffers", 10, NULL); // Лимит буфера
        GstElement *glupload1 = gst_element_factory_make ("glupload", NULL);
        GstElement *convert1 = gst_element_factory_make("glcolorconvert", NULL);

        // Элементы для второго потока
        GstElement *queue2 = gst_element_factory_make("queue", NULL);
        g_object_set(queue2, "max-size-buffers", 10, NULL);
        GstElement *decoder2 = gst_element_factory_make("mppvideodec", NULL);
        g_object_set(decoder2, "format", 11, NULL);
        GstElement *queue21 = gst_element_factory_make("queue", NULL); // Добавлено!
        g_object_set(queue21, "max-size-buffers", 10, NULL); // Лимит буфера
        GstElement *capsfilter2 = gst_element_factory_make ("capsfilter", NULL);
        GstCaps *caps2 = gst_caps_from_string ("video/x-raw,format=RGBA");
        g_object_set (capsfilter2, "caps", caps2, NULL);
        gst_clear_caps (&caps2);
        GstElement *glupload2 = gst_element_factory_make("glupload", NULL);
        GstElement *convert2 = gst_element_factory_make("glcolorconvert", NULL);

        // 3. Микшер и финальный приемник
        GstElement *mixer = gst_element_factory_make ("qml6glmixer", "mixer");

        GstElement *sink = gst_element_factory_make ("qml6glsink", NULL);

        g_assert(src && jitterbuffer && rtpdepay && parse && tee &&
                 queue1 && decoder1 && queue11 && glupload1 && convert1 &&
                 queue2 && decoder2 && queue21 && glupload2 && convert2 &&
                 mixer && sink);


        // 4. Добавляем элементы в конвейер
        gst_bin_add_many(GST_BIN(pipeline),
                         src, jitterbuffer, rtpdepay, parse, tee,
                         queue1, decoder1, queue11, glupload1, convert1,
                         queue2, decoder2, queue21, glupload2, convert2,
                         mixer, sink,
                         NULL);

        // 5. Соединяем основные элементы
        if (!gst_element_link_many(
                src,
                jitterbuffer,
                rtpdepay,
                parse,
                tee,    // Разделяем поток здесь
                NULL))
        {
            g_printerr("Ошибка соединения основных элементов!\n");
            return -1;
        }

        // 6. Соединяем первую ветку с конвертером
        if (!gst_element_link_many(
                queue1,
                decoder1,
                queue11,
                glupload1,
                convert1,
                NULL))
        {
            g_printerr("Ошибка соединения первой ветки!\n");
            return -1;
        }

        // 7. Соединяем вторую ветку с конвертером
        if (!gst_element_link_many(
                queue2,
                decoder2,
                queue21,
                glupload2,
                convert2,
                NULL))
        {
            g_printerr("Ошибка соединения второй ветки!\n");
            return -1;
        }

        // 6. Соединяем первую ветку с миксером
        if (!gst_element_link_many(
                convert1,
                sink,
                NULL))
        {
            g_printerr("Ошибка соединения первой ветки с миксером!\n");
            return -1;
        }

        // // 7. Соединяем вторую ветку с миксером
        // if (!gst_element_link_many(
        //         convert2,
        //         mixer,
        //         NULL))
        // {
        //     g_printerr("Ошибка соединения второй ветки с миксером!\n");
        //     return -1;
        // }


        // Запрашиваем площадки микшера
        mixer_pads->sinkpad1 = gst_element_get_static_pad(mixer, "sink_0");
        mixer_pads->sinkpad2 = gst_element_get_static_pad(mixer, "sink_1");


        // 9. Связываем tee с очередями
        GstPad *tee_src_pad1 = gst_element_request_pad_simple(tee, "src_%u");
        GstPad *queue_sink_pad1 = gst_element_get_static_pad(queue1, "sink");
        if (gst_pad_link(tee_src_pad1, queue_sink_pad1) != GST_PAD_LINK_OK) {
            g_printerr("Ошибка связи tee с queue1\n");
            return -1;
        }
        gst_object_unref(queue_sink_pad1);

        GstPad *tee_src_pad2 = gst_element_request_pad_simple(tee, "src_%u");
        GstPad *queue_sink_pad2 = gst_element_get_static_pad(queue2, "sink");
        if (gst_pad_link(tee_src_pad2, queue_sink_pad2) != GST_PAD_LINK_OK) {
            g_printerr("Ошибка связи tee с queue2\n");
            return -1;
        }
        gst_object_unref(queue_sink_pad2);


        // // 10. Связываем микшер с приемником
        // if (!gst_element_link(mixer, sink)) {
        //     g_printerr("Ошибка соединения микшера с приемником!\n");
        //     return -1;
        // }

        QQmlApplicationEngine engine;
        engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

        QQuickWindow *rootObject = static_cast<QQuickWindow *>(engine.rootObjects().first());
        QQuickItem *videoItem = rootObject->findChild<QQuickItem *>("videoItem");
        g_assert(videoItem);

        // Связываем финальный приемник с QML-элементом
        g_object_set(sink, "widget", videoItem, NULL);

        QDirIterator it(":", QDirIterator::Subdirectories);
        while (it.hasNext()) {
            qDebug() << it.next();
        }

        QFile f(":/mixer.qml");
        if(!f.open(QIODevice::ReadOnly)) {
            qWarning() << "error: " << f.errorString();
            return 1;
        }



        QByteArray overlay_scene = f.readAll();
        qDebug() << overlay_scene;

        // Обработчик инициализации сцены микшера
        g_signal_connect(mixer, "qml-scene-initialized", G_CALLBACK(on_mixer_scene_initialized), mixer_pads);
        g_object_set (mixer, "qml-scene", overlay_scene.data(), NULL);
        g_print("QML scene set to mixer\n");

        rootObject->scheduleRenderJob (new SetPlaying (pipeline),
                                      QQuickWindow::BeforeSynchronizingStage);

        ret = app.exec();

        gst_element_set_state (pipeline, GST_STATE_NULL);
        gst_object_unref (pipeline);
    }

    gst_deinit ();

    return ret;
}
