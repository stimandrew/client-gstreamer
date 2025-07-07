import QtQuick 6.0
import QtQuick.Window 6.0
import VideoRenderer 1.0

Window {
    id: window
    visible: true
    width: 1280
    height: 720  // Увеличиваем высоту окна, чтобы вместить три видео
    color: "black"

    Column {
        anchors.fill: parent
        spacing: 2

        // Верхний ряд (первые два видео)
        Row {
            width: parent.width
            height: parent.height / 2 - 1
            spacing: 2

            // Первый видео виджет
            Item {
                width: parent.width / 2 - 1
                height: parent.height

                VideoRenderer {
                    id: video1
                    objectName: "videoItem1"
                    anchors.fill: parent
                }
            }

            // Второй видео виджет
            Item {
                width: parent.width / 2 - 1
                height: parent.height

                VideoRenderer {
                    id: video2
                    objectName: "videoItem2"
                    anchors.fill: parent
                }
            }
        }

        // Нижний ряд (третье видео)
        Item {
            width: parent.width / 2 - 1  // Ширина как у одного из верхних видео
            height: parent.height / 2 - 1

            VideoRenderer {
                id: video3
                objectName: "videoItem3"
                anchors.fill: parent
            }
        }
    }
}
