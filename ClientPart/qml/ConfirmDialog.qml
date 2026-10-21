import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: dialog

    property string title: ""
    property string text: ""
    property string confirmText: "OK"
    property bool destructive: false

    signal confirmed

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 32, 360)
    modal: true
    focus: true
    padding: 20

    background: Rectangle {
        color: "#27272A"
        radius: 12
        border.color: "#3F3F46"
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Text {
            text: dialog.title
            color: "#F4F4F5"
            font.bold: true
            font.pixelSize: 18
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        Text {
            visible: dialog.text !== ""
            text: dialog.text
            color: "#A1A1AA"
            font.pixelSize: 14
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 8
            spacing: 10

            Button {
                id: cancelButton
                Layout.fillWidth: true
                text: "Отмена"
                background: Rectangle {
                    color: cancelButton.pressed ? "#3F3F46" : "#18181B"
                    radius: 8
                }
                contentItem: Text {
                    text: cancelButton.text
                    color: "#F4F4F5"
                    font.pixelSize: 15
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: dialog.close()
            }

            Button {
                id: okButton
                Layout.fillWidth: true
                text: dialog.confirmText
                background: Rectangle {
                    color: dialog.destructive ? (okButton.pressed ? "#B91C1C" : "#EF4444") : (okButton.pressed ? "#1D4ED8" : "#3B82F6")
                    radius: 8
                }
                contentItem: Text {
                    text: okButton.text
                    color: "#FFFFFF"
                    font.bold: true
                    font.pixelSize: 15
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    dialog.close();
                    dialog.confirmed();
                }
            }
        }
    }
}
