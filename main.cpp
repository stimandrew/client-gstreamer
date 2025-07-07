#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QCommandLineParser>
#include "videopipeline.h"
#include <QTimer>


int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "wayland");
    qputenv("GST_DEBUG", "4");
    qputenv("GST_DEBUG_NO_COLOR", "1");

    int ret;
    gst_init (&argc, &argv);

    {
        QGuiApplication app(argc, argv);


        // Настройка парсера командной строки
        QCommandLineParser parser;
        parser.setApplicationDescription("GStreamer UDP Video Client");
        parser.addHelpOption();

        QCommandLineOption portOption(
            QStringList() << "p" << "port",
            "UDP port",
            "port",
            "5000" // Значение по умолчанию
            );
        parser.addOption(portOption);
        parser.process(app);

        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

        VideoPipeline *pipeline1 = new VideoPipeline(5000);
        VideoPipeline *pipeline2 = new VideoPipeline(5001);
        if (!pipeline1->initialize() || !pipeline2->initialize()) {
            g_printerr("Ошибка инициализации pipeline!\n");
            return -1;
        }


        QQmlApplicationEngine engine;
        engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

        QQuickWindow *rootObject = static_cast<QQuickWindow*>(engine.rootObjects().first());
        QQuickItem *videoItem1 = rootObject->findChild<QQuickItem*>("videoItem1");
        QQuickItem *videoItem2 = rootObject->findChild<QQuickItem*>("videoItem2");

        QTimer::singleShot(100, [=]() {
            pipeline1->setVideoItem(videoItem1);
            pipeline2->setVideoItem(videoItem2);
            pipeline1->start();
            pipeline2->start();
        });

        ret = app.exec();

        // Очистка ресурсов
        pipeline1->stop();
        pipeline2->stop();
        delete pipeline1;
        delete pipeline2;


    }

    gst_deinit ();

    return ret;
}

