#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include "setplaying.h"
#include "videopipeline.h"


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

        VideoPipeline pipeline;
        if (!pipeline.initialize()) {
            g_printerr("Ошибка инициализации pipeline!\n");
            return -1;
        }

        QQmlApplicationEngine engine;
        engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

        QQuickItem *videoItem;
        QQuickWindow *rootObject;


        rootObject = static_cast<QQuickWindow *> (engine.rootObjects().first());
        videoItem = rootObject->findChild<QQuickItem *> ("videoItem");

        pipeline.setVideoItem(videoItem);
        rootObject->scheduleRenderJob(pipeline.createSetPlayingJob(),
                                      QQuickWindow::BeforeSynchronizingStage);
        ret = app.exec();


    }

    gst_deinit ();

    return ret;
}

