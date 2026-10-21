import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: serverPopup
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 32, 360)
    modal: true
    focus: true
    padding: 20

    onAboutToShow: {
        hostField.text = appState.serverHost;
        portField.text = appState.serverPort;
        errorText.visible = false;
    }

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
            text: "Сервер"
            color: "#F4F4F5"
            font.bold: true
            font.pixelSize: 18
        }

        TextField {
            id: hostField
            Layout.fillWidth: true
            placeholderText: "IP или домен"
            placeholderTextColor: "#71717A"
            color: "#F4F4F5"
            font.pixelSize: 15
            inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase
            background: Rectangle {
                color: "#18181B"
                radius: 8
                border.color: hostField.activeFocus ? "#3B82F6" : "#3F3F46"
                border.width: 1
            }
            Keys.onReturnPressed: connectButton.clicked()
        }

        TextField {
            id: portField
            Layout.fillWidth: true
            placeholderText: "Порт"
            placeholderTextColor: "#71717A"
            color: "#F4F4F5"
            font.pixelSize: 15
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator {
                bottom: 1
                top: 65535
            }
            background: Rectangle {
                color: "#18181B"
                radius: 8
                border.color: portField.activeFocus ? "#3B82F6" : "#3F3F46"
                border.width: 1
            }
            Keys.onReturnPressed: connectButton.clicked()
        }

        Text {
            id: errorText
            visible: false
            text: "Некорректный адрес или порт"
            color: "#EF4444"
            font.pixelSize: 13
        }

        RowLayout {
            Layout.fillWidth: true
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
                onClicked: serverPopup.close()
            }

            Button {
                id: connectButton
                Layout.fillWidth: true
                text: "Подключить"
                background: Rectangle {
                    color: connectButton.pressed ? "#1D4ED8" : "#3B82F6"
                    radius: 8
                }
                contentItem: Text {
                    text: connectButton.text
                    color: "#FFFFFF"
                    font.bold: true
                    font.pixelSize: 15
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    if (appController.changeServer(hostField.text, parseInt(portField.text)))
                        serverPopup.close();
                    else
                        errorText.visible = true;
                }
            }
        }
    }
}
