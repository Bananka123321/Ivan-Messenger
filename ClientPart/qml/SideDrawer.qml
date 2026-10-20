import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Drawer {
    id: drawer

    property bool swipeEnabled: false

    signal settingsRequested

    component DrawerItem: ItemDelegate {
        id: item
        property string glyph: ""
        property bool destructive: false

        Layout.fillWidth: true
        implicitHeight: 48
        leftPadding: 20

        background: Rectangle {
            color: item.pressed ? "#3F3F46" : (item.hovered ? "#2C2C30" : "transparent")
        }

        contentItem: RowLayout {
            spacing: 16

            Text {
                text: item.glyph
                font.pixelSize: 18
                color: item.destructive ? "#EF4444" : "#A1A1AA"
                Layout.preferredWidth: 24
                horizontalAlignment: Text.AlignHCenter
            }
            Text {
                text: item.text
                font.pixelSize: 15
                color: item.destructive ? "#EF4444" : "#F4F4F5"
                Layout.fillWidth: true
            }
        }
    }

    function todo(feature) {
        close();
        ApplicationWindow.window.todo(feature);
    }

    parent: Overlay.overlay
    width: Math.min(parent.width * 0.8, 300)
    height: parent.height
    edge: Qt.LeftEdge
    dragMargin: swipeEnabled ? Qt.styleHints.startDragDistance * 2 : 0

    background: Rectangle {
        color: "#202023"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            color: "#27272A"

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 20
                spacing: 6

                Avatar {
                    name: appState.username
                    size: 60
                }

                Text {
                    Layout.topMargin: 6
                    text: appState.username
                    color: "#F4F4F5"
                    font.bold: true
                    font.pixelSize: 17
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Text {
                    text: "ID: " + appState.userId
                    color: "#A1A1AA"
                    font.pixelSize: 13
                }
            }
        }

        Item {
            Layout.preferredHeight: 8
        }

        DrawerItem {
            glyph: "👤"
            text: "Мой профиль"
            onClicked: drawer.todo("Профиль")
        }
        DrawerItem {
            glyph: "👥"
            text: "Новая группа"
            onClicked: drawer.todo("Группы")
        }
        DrawerItem {
            glyph: "📇"
            text: "Контакты"
            onClicked: drawer.todo("Контакты")
        }
        DrawerItem {
            glyph: "🔖"
            text: "Избранное"
            onClicked: drawer.todo("Избранное")
        }
        DrawerItem {
            glyph: "📞"
            text: "Звонки"
            onClicked: drawer.todo("Звонки")
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            color: "#2C2C30"
        }

        DrawerItem {
            glyph: "⚙️"
            text: "Настройки"
            onClicked: {
                drawer.close();
                drawer.settingsRequested();
            }
        }
        DrawerItem {
            glyph: "🖧"
            text: "Сервер"
            onClicked: {
                drawer.close();
                serverDialog.open();
            }
        }

        Item {
            Layout.fillHeight: true
        }

        DrawerItem {
            glyph: "🚪"
            text: "Выйти"
            destructive: true
            onClicked: {
                drawer.close();
                logoutDialog.open();
            }
        }

        Text {
            Layout.margins: 20
            text: "Ivan Messenger"
            color: "#52525B"
            font.pixelSize: 12
        }
    }

    ServerDialog {
        id: serverDialog
    }

    ConfirmDialog {
        id: logoutDialog
        title: "Выйти из аккаунта?"
        confirmText: "Выйти"
        destructive: true
        onConfirmed: ApplicationWindow.window.logout()
    }
}
