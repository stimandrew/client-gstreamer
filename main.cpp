#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QCommandLineParser>
#include "videopipeline.h"
#include "videorenderer.h"
#include <QTimer>
#include <gst/gst.h>

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "wayland");
    qputenv("GST_DEBUG", "4");
    qputenv("GST_DEBUG_NO_COLOR", "1");

    int ret;
    gst_init(&argc, &argv);

    {
        QGuiApplication app(argc, argv);

        // Регистрируем наш VideoRenderer в QML
        qmlRegisterType<VideoRenderer>("VideoRenderer", 1, 0, "VideoRenderer");

        QCommandLineParser parser;
        parser.setApplicationDescription("GStreamer UDP Video Client");
        parser.addHelpOption();

        QCommandLineOption portOption(
            QStringList() << "p" << "port",
            "UDP port",
            "port",
            "5000");
        parser.addOption(portOption);
        parser.process(app);

        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

        VideoPipeline *pipeline1 = new VideoPipeline(5000);
        VideoPipeline *pipeline2 = new VideoPipeline(5001);
        if (!pipeline1->initialize() || !pipeline2->initialize()) {
            qWarning("Ошибка инициализации pipeline!");
            return -1;
        }

        QQmlApplicationEngine engine;
        engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

        QQuickWindow *rootObject = static_cast<QQuickWindow*>(engine.rootObjects().first());
        VideoRenderer *videoItem1 = rootObject->findChild<VideoRenderer*>("videoItem1");
        VideoRenderer *videoItem2 = rootObject->findChild<VideoRenderer*>("videoItem2");

        QObject::connect(pipeline1, &VideoPipeline::newFrame, videoItem1, &VideoRenderer::updateFrame);
        QObject::connect(pipeline2, &VideoPipeline::newFrame, videoItem2, &VideoRenderer::updateFrame);

        QTimer::singleShot(100, [=]() {
            pipeline1->start();
            pipeline2->start();
        });

        ret = app.exec();

        pipeline1->stop();
        pipeline2->stop();
        delete pipeline1;
        delete pipeline2;
    }

    gst_deinit();
    return ret;
}
