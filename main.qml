import QtQuick 6.0
import QtQuick.Window 6.0
import VideoRenderer 1.0

Window {
    id: window
    visible: true
    width: 1280
    height: 720
    color: "black"

    Column {
        anchors.fill: parent
        spacing: 2

        // Верхний ряд (первые два видео)
        Row {
                    width: parent.width
                    height: parent.height / 2 - 1
                    spacing: 2
                    anchors.horizontalCenter: parent.horizontalCenter
                    // Первый видео виджет
                    VideoRenderer {
                        id: video1
                        objectName: "videoItem1"
                        width: parent.width / 2 - 1
                        height: parent.height
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                    }

                    // Второй видео виджет
                    VideoRenderer {
                        id: video2
                        objectName: "videoItem2"
                        width: parent.width / 2 - 1
                        height: parent.height
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                    }
            }

        Row {
                    width: parent.width
                    height: parent.height / 2 - 1
                    spacing: 2

                    // Третий видео виджет
                    VideoRenderer {
                        id: video3
                        objectName: "videoItem3"
                        width: parent.width / 2 - 1
                        height: parent.height
                        anchors.verticalCenter: parent.verticalCenter
                    }
            }
    }
}
