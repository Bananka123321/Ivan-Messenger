import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

MenuItem {
    id: item

    property string glyph: ""
    property bool destructive: false

    height: visible ? implicitHeight : 0
    implicitHeight: 40
    leftPadding: 10
    rightPadding: 10

    background: Rectangle {
        radius: 6
        color: item.highlighted ? "#3F3F46" : "transparent"
    }

    contentItem: RowLayout {
        spacing: 12

        Text {
            text: item.glyph
            font.pixelSize: 16
            color: item.destructive ? "#EF4444" : "#A1A1AA"
            Layout.preferredWidth: 20
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            text: item.text
            font.pixelSize: 15
            color: item.destructive ? "#EF4444" : "#F4F4F5"
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
    }
}
