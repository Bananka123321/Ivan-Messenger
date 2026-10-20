import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: sheet

    property bool isMobile: false

    signal picked(string kind)

    function openAt(anchorItem) {
        if (isMobile) {
            x = 0;
            y = parent.height - implicitHeight;
        } else {
            const p = anchorItem.mapToItem(parent, 0, 0);
            x = Math.max(8, Math.min(p.x, parent.width - width - 8));
            y = p.y - implicitHeight - 8;
        }
        open();
    }

    parent: Overlay.overlay
    width: isMobile ? parent.width : 340
    modal: true
    focus: true
    padding: 16
    topPadding: isMobile ? 10 : 16
    bottomPadding: isMobile ? 24 : 16

    background: Rectangle {
        color: "#27272A"
        radius: 16
        border.color: "#3F3F46"
        border.width: sheet.isMobile ? 0 : 1

        Rectangle {
            visible: sheet.isMobile
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.radius
            color: parent.color
        }
    }

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: 120
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        Rectangle {
            visible: sheet.isMobile
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 36
            Layout.preferredHeight: 4
            radius: 2
            color: "#52525B"
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 3
            rowSpacing: 16
            columnSpacing: 8

            Repeater {
                model: [
                    { kind: "gallery", glyph: "🖼", title: "Галерея", color: "#3B82F6" },
                    { kind: "camera", glyph: "📷", title: "Камера", color: "#EC4899" },
                    { kind: "file", glyph: "📄", title: "Файл", color: "#8B5CF6" },
                    { kind: "location", glyph: "📍", title: "Геопозиция", color: "#10B981" },
                    { kind: "contact", glyph: "👤", title: "Контакт", color: "#F59E0B" },
                    { kind: "poll", glyph: "📊", title: "Опрос", color: "#14B8A6" }
                ]

                delegate: AbstractButton {
                    id: tile
                    required property var modelData

                    Layout.fillWidth: true
                    implicitHeight: 80
                    hoverEnabled: true

                    background: Rectangle {
                        radius: 10
                        color: tile.pressed ? "#3F3F46" : (tile.hovered ? "#2C2C30" : "transparent")
                    }

                    contentItem: ColumnLayout {
                        spacing: 6

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 52
                            Layout.preferredHeight: 52
                            radius: 26
                            color: tile.modelData.color

                            Text {
                                anchors.centerIn: parent
                                text: tile.modelData.glyph
                                font.pixelSize: 24
                            }
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: tile.modelData.title
                            color: "#E4E4E7"
                            font.pixelSize: 13
                        }
                    }

                    onClicked: {
                        sheet.close();
                        sheet.picked(tile.modelData.kind);
                    }
                }
            }
        }
    }
}
