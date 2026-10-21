import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#18181B"

    property bool isMobile: true

    signal backRequested

    component SettingsRow: ItemDelegate {
        id: row
        property string glyph: ""
        property string subtitle: ""
        property bool destructive: false

        Layout.fillWidth: true
        implicitHeight: subtitle !== "" ? 64 : 52
        leftPadding: 16
        rightPadding: 16

        background: Rectangle {
            color: row.pressed ? "#3F3F46" : (row.hovered ? "#2C2C30" : "transparent")
        }

        contentItem: RowLayout {
            spacing: 16

            Text {
                text: row.glyph
                font.pixelSize: 20
                color: row.destructive ? "#EF4444" : "#A1A1AA"
                Layout.preferredWidth: 28
                horizontalAlignment: Text.AlignHCenter
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: row.text
                    color: row.destructive ? "#EF4444" : "#F4F4F5"
                    font.pixelSize: 15
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Text {
                    visible: row.subtitle !== ""
                    text: row.subtitle
                    color: "#71717A"
                    font.pixelSize: 13
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            Text {
                visible: !row.destructive
                text: "›"
                color: "#52525B"
                font.pixelSize: 22
            }
        }
    }

    component SectionTitle: Text {
        Layout.leftMargin: 16
        Layout.topMargin: 16
        Layout.bottomMargin: 4
        color: "#3B82F6"
        font.bold: true
        font.pixelSize: 13
    }

    function todo(feature) {
        ApplicationWindow.window.todo(feature);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#27272A"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 4
                anchors.rightMargin: 8
                spacing: 8

                IconButton {
                    visible: root.isMobile
                    glyph: "←"
                    glyphSize: 22
                    glyphColor: "#F4F4F5"
                    onClicked: root.backRequested()
                }

                Text {
                    Layout.leftMargin: root.isMobile ? 0 : 12
                    text: "Настройки"
                    color: "#F4F4F5"
                    font.bold: true
                    font.pixelSize: 18
                    Layout.fillWidth: true
                }

                IconButton {
                    visible: !root.isMobile
                    glyph: "✕"
                    glyphSize: 18
                    onClicked: root.backRequested()
                }
            }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentHeight: content.implicitHeight
            boundsBehavior: Flickable.StopAtBounds

            ColumnLayout {
                id: content
                width: parent.width
                spacing: 0

                AbstractButton {
                    id: profileCard
                    Layout.fillWidth: true
                    implicitHeight: 96
                    hoverEnabled: true

                    background: Rectangle {
                        color: profileCard.pressed ? "#3F3F46" : (profileCard.hovered ? "#2C2C30" : "transparent")
                    }

                    contentItem: RowLayout {
                        spacing: 16

                        Avatar {
                            Layout.leftMargin: 16
                            name: appState.username
                            size: 64
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: appState.username
                                color: "#F4F4F5"
                                font.bold: true
                                font.pixelSize: 18
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Text {
                                text: "ID: " + appState.userId
                                color: "#A1A1AA"
                                font.pixelSize: 13
                            }
                        }

                        IconButton {
                            Layout.rightMargin: 12
                            glyph: "📷"
                            tip: "Сменить фото"
                            onClicked: root.todo("Смена аватара")
                        }
                    }

                    onClicked: root.todo("Редактирование профиля")
                }

                SectionTitle {
                    text: "Основные"
                }
                SettingsRow {
                    glyph: "🔔"
                    text: "Уведомления и звуки"
                    onClicked: root.todo("Настройки уведомлений")
                }
                SettingsRow {
                    glyph: "🔒"
                    text: "Конфиденциальность"
                    subtitle: "Кто видит мой статус, кто может писать"
                    onClicked: root.todo("Конфиденциальность")
                }
                SettingsRow {
                    glyph: "🎨"
                    text: "Оформление"
                    subtitle: "Тёмная тема"
                    onClicked: root.todo("Настройки оформления")
                }
                SettingsRow {
                    glyph: "💬"
                    text: "Чаты"
                    subtitle: "Размер текста, фон, отправка по Enter"
                    onClicked: root.todo("Настройки чатов")
                }
                SettingsRow {
                    glyph: "💾"
                    text: "Данные и память"
                    subtitle: "Автозагрузка медиа, кэш"
                    onClicked: root.todo("Данные и память")
                }
                SettingsRow {
                    glyph: "🌐"
                    text: "Язык"
                    subtitle: "Русский"
                    onClicked: root.todo("Смена языка")
                }

                SectionTitle {
                    text: "Аккаунт"
                }
                SettingsRow {
                    glyph: "💻"
                    text: "Устройства"
                    subtitle: "Активные сеансы"
                    onClicked: root.todo("Активные сеансы")
                }
                SettingsRow {
                    glyph: "🖧"
                    text: "Сервер"
                    subtitle: appState.serverHost + ":" + appState.serverPort
                    onClicked: serverDialog.open()
                }
                SettingsRow {
                    glyph: "ℹ️"
                    text: "О приложении"
                    onClicked: root.todo("О приложении")
                }
                SettingsRow {
                    glyph: "🚪"
                    text: "Выйти"
                    destructive: true
                    onClicked: logoutDialog.open()
                }

                Item {
                    Layout.preferredHeight: 24
                }
            }
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
