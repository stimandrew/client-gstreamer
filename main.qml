import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import VideoRenderer 1.0
import VideoController 1.0

Window {
    visible: true
    width: 1280
    height: 720
    color: "black"

    Column {
        anchors.fill: parent
        spacing: 2

        Row {
            width: parent.width
            height: parent.height / 2 - 1
            spacing: 2

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

                    Button {
                        text: controller1.isRunning ? "Stop 1" : "Start 1"
                        onClicked: controller1.isRunning ? controller1.stop() : controller1.start()
                    }

                    TextField {
                        width: 100
                        height: parent.height
                        placeholderText: "Port"
                        text: controller1.port
                        validator: IntValidator { bottom: 0; top: 65535 }
                        onAccepted: controller1.setPort(parseInt(text))
                    }

                    CheckBox {
                        text: "YOLO"
                        checked: controller1.yoloEnabled
                        onCheckedChanged: controller1.setYoloEnabled(checked)
                    }

                    Button {
                        text: "Load Model"
                        onClicked: controller1.setYoloModelPath("yolo11n.rknn")
                    }
                }
            }

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

                    Button {
                        text: controller2.isRunning ? "Stop 2" : "Start 2"
                        onClicked: controller2.isRunning ? controller2.stop() : controller2.start()
                    }

                    TextField {
                        width: 100
                        height: parent.height
                        placeholderText: "Port"
                        text: controller2.port
                        validator: IntValidator { bottom: 0; top: 65535 }
                        onAccepted: controller2.setPort(parseInt(text))
                    }
                }
            }
        }

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
                    showObjects: true
                }
            }

            Row {
                width: parent.width
                height: 50
                spacing: 10

                Button {
                    text: controller3.isRunning ? "Stop 3" : "Start 3"
                    onClicked: controller3.isRunning ? controller3.stop() : controller3.start()
                }

                TextField {
                    width: 100
                    height: parent.height
                    placeholderText: "Port"
                    text: controller3.port
                    validator: IntValidator { bottom: 0; top: 65535 }
                    onAccepted: controller3.setPort(parseInt(text))
                }
            }
        }
    }
}
