#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext> // Добавлено для работы с rootContext
#include <QQuickWindow>
#include <QQuickItem>
#include <QCommandLineParser>
#include "videorenderer.h"
#include "videocontroller.h"
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

        // Регистрируем наши классы в QML
        qmlRegisterType<VideoRenderer>("VideoRenderer", 1, 0, "VideoRenderer");
        qmlRegisterType<VideoController>("VideoController", 1, 0, "VideoController");

        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

        QQmlApplicationEngine engine;

        // Создаем контроллеры и устанавливаем порты
        VideoController *controller1 = new VideoController(&engine);
        VideoController *controller2 = new VideoController(&engine);
        VideoController *controller3 = new VideoController(&engine);

        controller1->setPort(5000);
        controller2->setPort(5001);
        controller3->setPort(5002);

        // Регистрируем контроллеры в контексте QML
        engine.rootContext()->setContextProperty("controller1", controller1);
        engine.rootContext()->setContextProperty("controller2", controller2);
        engine.rootContext()->setContextProperty("controller3", controller3);

        engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

        QQuickWindow *rootObject = static_cast<QQuickWindow*>(engine.rootObjects().first());
        VideoRenderer *videoItem1 = rootObject->findChild<VideoRenderer*>("videoItem1");
        VideoRenderer *videoItem2 = rootObject->findChild<VideoRenderer*>("videoItem2");
        VideoRenderer *videoItem3 = rootObject->findChild<VideoRenderer*>("videoItem3");

        if (videoItem1 && videoItem2 && videoItem3) {
            QObject::connect(controller1, &VideoController::newFrame, videoItem1, &VideoRenderer::updateFrame);
            QObject::connect(controller2, &VideoController::newFrame, videoItem2, &VideoRenderer::updateFrame);
            QObject::connect(controller3, &VideoController::newFrame, videoItem3, &VideoRenderer::updateFrame);
        } else {
            qWarning() << "Failed to find video items";
        }

        ret = app.exec();

        delete controller1;
        delete controller2;
        delete controller3;
    }

    gst_deinit();
    return ret;
}
