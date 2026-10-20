import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: panel

    property bool isMobile: false

    signal emojiPicked(string emoji)
    signal stickersRequested
    signal gifRequested

    readonly property var emojis: ["😀", "😃", "😄", "😁", "😆", "😅", "😂", "🤣", "😊", "😇", "🙂", "😉", "😌", "😍", "🥰", "😘", "😋", "😛", "😜", "🤪", "🤨", "🧐", "🤓", "😎", "🥳", "😏", "😒", "😞", "😔", "😟", "😕", "🙁", "😣", "😖", "😫", "😩", "🥺", "😢", "😭", "😤", "😠", "😡", "🤯", "😳", "🥵", "🥶", "😱", "😨", "🤔", "🤭", "🤫", "😶", "😐", "😑", "😬", "🙄", "😯", "😴", "🤤", "😵", "🤐", "🤢", "🤮", "🤧", "😷", "🤒", "🤕", "🤑", "🤠", "😈", "👻", "💀", "🤡", "💩", "👍", "👎", "👌", "✌", "🤞", "🤝", "👏", "🙌", "🙏", "💪", "👋", "🤙", "❤", "🧡", "💛", "💚", "💙", "💜", "🖤", "💔", "🔥", "✨", "⭐", "🎉", "🎁", "✅", "❌", "❓", "💯", "👀"]

    function openAt(anchorItem) {
        if (isMobile) {
            x = 0;
            y = parent.height - height;
        } else {
            const p = anchorItem.mapToItem(parent, anchorItem.width, 0);
            x = Math.max(8, Math.min(p.x - width, parent.width - width - 8));
            y = p.y - height - 8;
        }
        open();
    }

    parent: Overlay.overlay
    width: isMobile ? parent.width : 340
    height: 320
    modal: true
    padding: 8

    background: Rectangle {
        color: "#27272A"
        radius: 14
        border.color: "#3F3F46"
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            cellWidth: width / Math.max(1, Math.floor(width / 42))
            cellHeight: 42
            model: panel.emojis

            delegate: AbstractButton {
                id: cell
                required property string modelData

                width: grid.cellWidth
                height: grid.cellHeight
                hoverEnabled: true

                background: Rectangle {
                    radius: 8
                    color: cell.pressed ? "#3F3F46" : (cell.hovered ? "#2C2C30" : "transparent")
                }
                contentItem: Text {
                    text: cell.modelData
                    font.pixelSize: 24
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: panel.emojiPicked(cell.modelData)
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#3F3F46"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            IconButton {
                glyph: "😊"
                tip: "Эмодзи"
                bgColor: "#3F3F46"
            }
            IconButton {
                glyph: "🎭"
                tip: "Стикеры"
                onClicked: panel.stickersRequested()
            }
            IconButton {
                glyph: "GIF"
                glyphSize: 13
                glyphColor: "#E4E4E7"
                tip: "GIF"
                onClicked: panel.gifRequested()
            }
            Item {
                Layout.fillWidth: true
            }
        }
    }
}
