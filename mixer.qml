import QtQuick 6.0
import org.freedesktop.gstreamer.Qt6GLVideoItem 1.0

Item {
    anchors.fill: parent

    GstGLQt6VideoItem {
        id: video0
        objectName: "inputVideoItem0"
        anchors {
            left: parent.left
            right: parent.horizontalCenter
            top: parent.top
            bottom: parent.bottom
        }
    }

    GstGLQt6VideoItem {
        id: video1
        objectName: "inputVideoItem1"
        anchors {
            left: parent.horizontalCenter
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
    }
}
