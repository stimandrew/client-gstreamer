import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Dialogs 6.0
import QtQuick.Window 6.0

import org.freedesktop.gstreamer.Qt6GLVideoItem 1.0

Window {
    id: window
    visible: true
    width: 1000
    height: 500
    x: 30
    y: 30
    color: "black"

        Item {
            anchors.fill: parent

            GstGLQt6VideoItem {
                id: video
                objectName: "videoItem"
                anchors.centerIn: parent
                width: parent.width
                height: parent.height
            }

            Text {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: "qmlglsink text"
                font.pointSize: 20
                color: "yellow"
                style: Text.Outline
                styleColor: "blue"
            }
        }
}
