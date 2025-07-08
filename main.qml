import QtQuick 6.0
import QtQuick.Window 6.0
import QtQuick.Controls 6.0
import VideoRenderer 1.0
import VideoController 1.0

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
            Column {
                width: parent.width / 2 - 1
                height: parent.height
                spacing: 5

                Rectangle {
                    width: parent.width
                    height: parent.height - 50
                    color: controller1.isRunning ? "transparent" : "black"

                    VideoRenderer {
                        id: video1
                        objectName: "videoItem1"
                        width: parent.width
                        height: parent.height
                        visible: controller1.isRunning
                    }
                }

                Row {
                    width: parent.width
                    height: 50
                    spacing: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                    layoutDirection: Qt.LeftToRight

                    Button {
                        text: controller1.isRunning ? "Stop 1" : "Start 1"
                        onClicked: {
                            if (controller1.isRunning) {
                                controller1.stop();
                            } else {
                                controller1.start();
                            }
                        }
                    }

                    Text {
                        id: textPort1
                        text: "Port: " + controller1.port
                        color: "white"
                        height: parent.height
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            // Второй видео виджет
            Column {
                width: parent.width / 2 - 1
                height: parent.height
                spacing: 5

                Rectangle {
                    width: parent.width
                    height: parent.height - 50
                    color: controller2.isRunning ? "transparent" : "black"

                    VideoRenderer {
                        id: video2
                        objectName: "videoItem2"
                        width: parent.width
                        height: parent.height
                        visible: controller2.isRunning
                    }
                }

                Row {
                    width: parent.width
                    height: 50
                    spacing: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                    layoutDirection: Qt.LeftToRight

                    Button {
                        text: controller2.isRunning ? "Stop 2" : "Start 2"
                        onClicked: {
                            if (controller2.isRunning) {
                                controller2.stop();
                            } else {
                                controller2.start();
                            }
                        }
                    }

                    Text {
                        id: textPort2
                        text: "Port: " + controller2.port
                        color: "white"
                        height: parent.height
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        // Нижний ряд (третий видео)
        Column {
            width: parent.width / 2 - 1
            height: parent.height / 2 - 1
            spacing: 5
            anchors.horizontalCenter: parent.horizontalCenter

            Rectangle {
                width: parent.width
                height: parent.height - 50
                color: controller3.isRunning ? "transparent" : "black"

                VideoRenderer {
                    id: video3
                    objectName: "videoItem3"
                    width: parent.width
                    height: parent.height
                    visible: controller3.isRunning
                }
            }

            Row {
                width: parent.width
                height: 50
                spacing: 10
                anchors.horizontalCenter: parent.horizontalCenter
                layoutDirection: Qt.LeftToRight

                Button {
                    text: controller3.isRunning ? "Stop 3" : "Start 3"
                    onClicked: {
                        if (controller3.isRunning) {
                            controller3.stop();
                        } else {
                            controller3.start();
                        }
                    }
                }

                Text {
                    id: textPort3
                    text: "Port: " + controller3.port
                    color: "white"
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
