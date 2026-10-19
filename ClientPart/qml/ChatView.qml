import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#18181B"

    property int currentChatId: 0
    property string currentChatName: "Выберите чат"
    property bool isMobile: false

    signal backRequested

    function openChat(chatId, chatName) {
        currentChatName = chatName;
        currentChatId = chatId;
        reloadMessages();
    }

    function reloadMessages() {
        msgModel.clear();
        if (!dialogManager || currentChatId === 0)
            return;
        const list = dialogManager.messagesForQml(currentChatId);
        for (let i = 0; i < list.length; ++i)
            msgModel.append(list[i]);
    }

    Component.onCompleted: reloadMessages()

    Connections {
        target: dialogManager
        function onMessagesUpdated(dialogId) {
            if (dialogId === root.currentChatId)
                root.reloadMessages();
        }
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
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Button {
                    visible: root.isMobile
                    text: "←"
                    font.pixelSize: 20
                    font.bold: true
                    background: Rectangle {
                        color: "transparent"
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#F4F4F5"
                        font: parent.font
                    }
                    onClicked: root.backRequested()
                    Layout.preferredWidth: 40
                }

                Text {
                    text: root.currentChatName
                    color: "#F4F4F5"
                    font.bold: true
                    font.pixelSize: 18
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#18181B"

            ListView {
                id: messagesView
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8
                clip: true
                model: ListModel {
                    id: msgModel
                }
                verticalLayoutDirection: ListView.BottomToTop

                delegate: Rectangle {
                    required property string messageText
                    required property bool isMine

                    width: Math.min(messagesView.width * 0.7, msg.implicitWidth + 24)
                    height: msg.paintedHeight + 24
                    radius: 12
                    color: isMine ? "#3B82F6" : "#27272A"
                    x: isMine ? messagesView.width - width : 0

                    Text {
                        id: msg
                        text: parent.messageText
                        color: "#F4F4F5"
                        font.pixelSize: 15
                        wrapMode: Text.Wrap
                        width: Math.min(implicitWidth, messagesView.width * 0.7 - 24)
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: 12
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            color: "#27272A"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                TextField {
                    id: messageInput
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    placeholderText: "Написать сообщение..."
                    placeholderTextColor: "#71717A"
                    color: "#F4F4F5"
                    font.pixelSize: 15
                    background: Rectangle {
                        color: "#18181B"
                        radius: 20
                        border.color: messageInput.activeFocus ? "#3B82F6" : "#3F3F46"
                        border.width: 1
                    }
                    Keys.onReturnPressed: sendButton.clicked()
                }

                Button {
                    id: sendButton
                    text: "➤"
                    font.pixelSize: 20
                    Layout.preferredWidth: 50
                    Layout.preferredHeight: 50
                    background: Rectangle {
                        color: parent.pressed ? "#1D4ED8" : "#3B82F6"
                        radius: 25
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#FFFFFF"
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (messageInput.text.trim() !== "" && root.currentChatId !== 0) {
                            appController.sendText(root.currentChatId, messageInput.text);
                            messageInput.text = "";
                        }
                    }
                }
            }
        }
    }
}
