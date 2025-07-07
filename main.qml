import QtQuick 6.0
import QtQuick.Window 6.0
import VideoRenderer 1.0

Window {
    id: window
    visible: true
    width: 1280
    height: 480
    color: "black"

    Row {
        anchors.fill: parent
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
}
