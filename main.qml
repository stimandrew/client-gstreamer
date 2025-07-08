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

                    TextField {
                        id: portInput1
                        width: 100
                        height: parent.height
                        placeholderText: "Port"
                        text: controller1.port
                        validator: IntValidator { bottom: 0; top: 65535 }
                        color: "white"
                        background: Rectangle { color: "darkgray" }
                    }

                    Button {
                        text: "Set Port"
                        onClicked: {
                            var newPort = parseInt(portInput1.text);
                            if (newPort >= 0 && newPort <= 65535) {
                                controller1.setPort(newPort);
                                if (controller1.isRunning) {
                                    controller1.start();
                                }
                            } else {
                                console.log("Invalid port number");
                            }
                        }
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

                    TextField {
                        id: portInput2
                        width: 100
                        height: parent.height
                        placeholderText: "Port"
                        text: controller2.port
                        validator: IntValidator { bottom: 0; top: 65535 }
                        color: "white"
                        background: Rectangle { color: "darkgray" }
                    }

                    Button {
                        text: "Set Port"
                        onClicked: {
                            controller2.setPort(parseInt(portInput2.text));
                            if (controller2.isRunning) {
                                controller2.stop();
                                controller2.start();
                            }
                        }
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

                TextField {
                    id: portInput3
                    width: 100
                    height: parent.height
                    placeholderText: "Port"
                    text: controller3.port
                    validator: IntValidator { bottom: 0; top: 65535 }
                    color: "white"
                    background: Rectangle { color: "darkgray" }
                }

                Button {
                    text: "Set Port"
                    onClicked: {
                        controller3.setPort(parseInt(portInput3.text));
                        if (controller3.isRunning) {
                            controller3.stop();
                            controller3.start();
                        }
                    }
                }
            }
        }
    }
}
