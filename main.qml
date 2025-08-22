import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15
import VideoRenderer 1.0
import VideoController 1.0

Window {
    id: mainWindow
    visible: true
    width: 1920
    height: 1080
    color: "black"

    Connections {
        target: controller1
        function onModbusErrorOccurred(error) {
            console.log("Modbus error:", error)
            errorPopup.text = error
            errorPopup.open()
        }
    }

    Popup {
        id: errorPopup
        x: 100
        y: 100
        width: 400
        height: 100
        modal: true
        focus: true

        Text {
            id: errorText
            anchors.fill: parent
            anchors.margins: 10
            text: "Modbus Error"
            color: "red"
            wrapMode: Text.Wrap
        }

        onClosed: {
            errorText.text = ""
        }
    }

    Grid {
        anchors.fill: parent
        columns: 2
        rows: 2
        spacing: 2

        // Верхняя левая ячейка - Видео 1
        Rectangle {
            width: parent.width / 2 - 1
            height: parent.height / 2 - 1
            color: controller1.isRunning ? "transparent" : "black"

            VideoRenderer {
                id: video1
                objectName: "videoItem1"
                anchors.fill: parent
                visible: controller1.isRunning
            }

            Text {
                id: fpsText1
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 10
                color: "white"
                font.pixelSize: 20
                text: "FPS: " + controller1.fps
            }

            Canvas {
                id: detectionCanvas
                anchors.fill: parent
                visible: controller1.yoloEnabled

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = "red"
                    ctx.lineWidth = 2
                    ctx.font = "14px Sans Serif"
                    ctx.fillStyle = "red"

                    var scaleX = width / video1.width
                    var scaleY = height / video1.height

                    for (var i = 0; i < controller1.objects.length; i++) {
                        var obj = controller1.objects[i]
                        var rect = obj.rect
                        var label = obj.label

                        var scaledX = rect.x * scaleX
                        var scaledY = rect.y * scaleY
                        var scaledWidth = rect.width * scaleX
                        var scaledHeight = rect.height * scaleY

                        ctx.strokeRect(scaledX, scaledY, scaledWidth, scaledHeight)
                        ctx.fillText(label, scaledX + 5, scaledY + 20)
                    }
                }
            }

            Connections {
                target: controller1
                function onObjectsChanged() {
                    detectionCanvas.requestPaint()
                }
            }

            // Управление видео 1
            Column {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 5
                spacing: 5

                Row {
                    width: parent.width
                    height: 40
                    spacing: 10

                    Button {
                        text: controller1.isRunning ? "Stop 1" : "Start 1"
                        onClicked: controller1.isRunning ? controller1.stop() : controller1.start()
                    }

                    TextField {
                        width: 80
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
        }

        // Верхняя правая ячейка - Видео 2
        Rectangle {
            width: parent.width / 2 - 1
            height: parent.height / 2 - 1
            color: controller2.isRunning ? "transparent" : "black"

            VideoRenderer {
                id: video2
                objectName: "videoItem2"
                anchors.fill: parent
                visible: controller2.isRunning
            }

            Text {
                id: fpsText2
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 10
                color: "white"
                font.pixelSize: 20
                text: "FPS: " + controller2.fps
            }

            Canvas {
                id: detectionCanvas2
                anchors.fill: parent
                visible: controller2.yoloEnabled

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = "red"
                    ctx.lineWidth = 2
                    ctx.font = "14px Sans Serif"
                    ctx.fillStyle = "red"

                    var scaleX = width / video2.width
                    var scaleY = height / video2.height

                    for (var i = 0; i < controller2.objects.length; i++) {
                        var obj = controller2.objects[i]
                        var rect = obj.rect
                        var label = obj.label

                        var scaledX = rect.x * scaleX
                        var scaledY = rect.y * scaleY
                        var scaledWidth = rect.width * scaleX
                        var scaledHeight = rect.height * scaleY

                        ctx.strokeRect(scaledX, scaledY, scaledWidth, scaledHeight)
                        ctx.fillText(label, scaledX + 5, scaledY + 20)
                    }
                }
            }

            Connections {
                target: controller2
                function onObjectsChanged() {
                    detectionCanvas2.requestPaint()
                }
            }

            // Управление видео 2
            Column {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 5
                spacing: 5

                Row {
                    width: parent.width
                    height: 40
                    spacing: 10

                    Button {
                        text: controller2.isRunning ? "Stop 2" : "Start 2"
                        onClicked: controller2.isRunning ? controller2.stop() : controller2.start()
                    }

                    TextField {
                        width: 80
                        height: parent.height
                        placeholderText: "Port"
                        text: controller2.port
                        validator: IntValidator { bottom: 0; top: 65535 }
                        onAccepted: controller2.setPort(parseInt(text))
                    }

                    CheckBox {
                        text: "YOLO"
                        checked: controller2.yoloEnabled
                        onCheckedChanged: controller2.setYoloEnabled(checked)
                    }

                    Button {
                        text: "Load Model"
                        onClicked: controller2.setYoloModelPath("yolo11n.rknn")
                    }
                }
            }
        }

        // Нижняя левая ячейка - Видео 3
        Rectangle {
            width: parent.width / 2 - 1
            height: parent.height / 2 - 1
            color: controller3.isRunning ? "transparent" : "black"

            VideoRenderer {
                id: video3
                objectName: "videoItem3"
                anchors.fill: parent
                visible: controller3.isRunning
                showObjects: true
            }

            Text {
                id: fpsText3
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 10
                color: "white"
                font.pixelSize: 20
                text: "FPS: " + controller3.fps
            }

            Canvas {
                id: detectionCanvas3
                anchors.fill: parent
                visible: controller3.yoloEnabled

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = "red"
                    ctx.lineWidth = 2
                    ctx.font = "14px Sans Serif"
                    ctx.fillStyle = "red"

                    var scaleX = width / video3.width
                    var scaleY = height / video3.height

                    for (var i = 0; i < controller3.objects.length; i++) {
                        var obj = controller3.objects[i]
                        var rect = obj.rect
                        var label = obj.label

                        var scaledX = rect.x * scaleX
                        var scaledY = rect.y * scaleY
                        var scaledWidth = rect.width * scaleX
                        var scaledHeight = rect.height * scaleY

                        ctx.strokeRect(scaledX, scaledY, scaledWidth, scaledHeight)
                        ctx.fillText(label, scaledX + 5, scaledY + 20)
                    }
                }
            }

            Connections {
                target: controller3
                function onObjectsChanged() {
                    detectionCanvas3.requestPaint()
                }
            }

            // Управление видео 3
            Column {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 5
                spacing: 5

                Row {
                    width: parent.width
                    height: 40
                    spacing: 10

                    Button {
                        text: controller3.isRunning ? "Stop 3" : "Start 3"
                        onClicked: controller3.isRunning ? controller3.stop() : controller3.start()
                    }

                    TextField {
                        width: 80
                        height: parent.height
                        placeholderText: "Port"
                        text: controller3.port
                        validator: IntValidator { bottom: 0; top: 65535 }
                        onAccepted: controller3.setPort(parseInt(text))
                    }

                    CheckBox {
                        text: "YOLO"
                        checked: controller3.yoloEnabled
                        onCheckedChanged: controller3.setYoloEnabled(checked)
                    }

                    Button {
                        text: "Load Model"
                        onClicked: controller3.setYoloModelPath("yolo11n.rknn")
                    }
                }
            }
        }

        // Нижняя правая ячейка - Modbus интерфейс
        Rectangle {
            width: parent.width / 2 - 1
            height: parent.height / 2 - 1
            color: "#2d2d2d"
            border.color: "gray"
            border.width: 1

            Column {
                anchors.centerIn: parent
                width: parent.width * 0.8
                spacing: 15

                Text {
                    text: "Modbus Control"
                    color: "white"
                    font.pixelSize: 18
                    font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // Индикатор соединения
                Row {
                    width: parent.width
                    spacing: 10
                    anchors.horizontalCenter: parent.horizontalCenter

                    Rectangle {
                        width: 20
                        height: 20
                        radius: 10
                        color: controller1.modbusConnected ? "green" : "red"
                        border.color: "white"
                        border.width: 2

                        ToolTip.visible: ma.containsMouse
                        ToolTip.text: controller1.modbusConnected ? "Modbus connected" : "Modbus disconnected"

                        MouseArea {
                            id: ma
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                    }

                    Text {
                        text: controller1.modbusConnected ? "✓ Connected" : "✗ Disconnected"
                        color: controller1.modbusConnected ? "lightgreen" : "lightcoral"
                        font.pixelSize: 16
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                // Поле ввода адреса
                Column {
                    width: parent.width
                    spacing: 5

                    Text {
                        text: "Modbus Server:"
                        color: "white"
                        font.pixelSize: 14
                    }

                    TextField {
                        width: parent.width
                        placeholderText: "IP:Port (e.g., 192.168.1.100:502)"
                        text: controller1.modbusAddress
                        onAccepted: controller1.setModbusAddress(text)
                    }
                }

                // Кнопки управления
                Row {
                    width: parent.width
                    spacing: 10
                    anchors.horizontalCenter: parent.horizontalCenter

                    Button {
                        text: "Connect"
                        enabled: !controller1.modbusConnected
                        onClicked: controller1.connectModbus()
                    }

                    Button {
                        text: "Disconnect"
                        enabled: controller1.modbusConnected
                        onClicked: controller1.disconnectModbus()
                    }
                }

                // Статусная информация
                Rectangle {
                    width: parent.width
                    height: 60
                    color: "#3d3d3d"
                    radius: 5

                    Text {
                        anchors.fill: parent
                        anchors.margins: 5
                        text: controller1.modbusConnected ?
                              "Connected to: " + controller1.modbusAddress :
                              "Enter Modbus server address and click Connect"
                        color: "lightgray"
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                Column {
                    width: parent.width
                    spacing: 15

                    Button {
                        text: "Reboot System"
                        enabled: controller1.modbusConnected
                        onClicked: {
                            var component = Qt.createComponent("ConfirmDialog.qml");
                            if (component.status === Component.Ready) {
                                var dialog = component.createObject(mainWindow);
                                dialog.confirmed.connect(function() {
                                    controller1.sendRebootCommand();
                                });
                                dialog.open();
                            } else {
                                console.error("Error loading component:", component.errorString());
                            }
                        }
                        ToolTip.visible: hovered
                        ToolTip.text: "Send reboot command to server"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    GroupBox {
                        title: "Modbus Camera Control"
                        width: parent.width

                        ColumnLayout {
                            width: parent.width
                            spacing: 10

                            RowLayout {
                                Label {
                                    text: "Camera 1:"
                                    color: controller1.camera1Active ? "lightgreen" : "white"
                                }
                                Switch {
                                    id: cam1Switch
                                    enabled: controller1.modbusConnected
                                    checked: controller1.camera1Active
                                    onCheckedChanged: {
                                        if (controller1.modbusConnected && checked !== controller1.camera1Active) {
                                            controller1.writeCoil(0, checked)
                                        }
                                    }
                                }
                                Rectangle {
                                    width: 12
                                    height: 12
                                    radius: 6
                                    color: controller1.camera1Active ? "green" : "red"
                                }
                            }

                            RowLayout {
                                Label {
                                    text: "Camera 2:"
                                    color: controller1.camera2Active ? "lightgreen" : "white"
                                }
                                Switch {
                                    id: cam2Switch
                                    enabled: controller1.modbusConnected
                                    checked: controller1.camera2Active
                                    onCheckedChanged: {
                                        if (controller1.modbusConnected && checked !== controller1.camera2Active) {
                                            controller1.writeCoil(1, checked)
                                        }
                                    }
                                }
                                Rectangle {
                                    width: 12
                                    height: 12
                                    radius: 6
                                    color: controller1.camera2Active ? "green" : "red"
                                }
                            }

                            RowLayout {
                                Label {
                                    text: "Camera 3:"
                                    color: controller1.camera3Active ? "lightgreen" : "white"
                                }
                                Switch {
                                    id: cam3Switch
                                    enabled: controller1.modbusConnected
                                    checked: controller1.camera3Active
                                    onCheckedChanged: {
                                        if (controller1.modbusConnected && checked !== controller1.camera3Active) {
                                            controller1.writeCoil(2, checked)
                                        }
                                    }
                                }
                                Rectangle {
                                    width: 12
                                    height: 12
                                    radius: 6
                                    color: controller1.camera3Active ? "green" : "red"
                                }
                            }

                            Button {
                                text: "Refresh States"
                                enabled: controller1.modbusConnected
                                onClicked: controller1.readCameraStates()
                                Layout.alignment: Qt.AlignCenter
                            }
                        }
                    }
                }
            }
        }
    }
}
