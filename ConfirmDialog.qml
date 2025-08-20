// ConfirmDialog.qml
import QtQuick 2.15
import QtQuick.Controls 2.15

Dialog {
    id: confirmDialog
    title: "Confirm Reboot"
    standardButtons: Dialog.Yes | Dialog.No
    modal: true
    width: 400
    height: 150
    x: (parent ? (parent.width - width) / 2 : 0)
    y: (parent ? (parent.height - height) / 2 : 0)

    property alias message: label.text

    signal confirmed()
    signal cancelled()

    Label {
        id: label
        anchors.centerIn: parent
        text: "Are you sure you want to reboot the system?"
        wrapMode: Text.Wrap
    }

    onAccepted: confirmed()
    onRejected: cancelled()
}
