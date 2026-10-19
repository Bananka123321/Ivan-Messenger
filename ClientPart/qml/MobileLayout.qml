import QtQuick
import QtQuick.Controls

Item {
    function showChat(chatId, chatName) {
        const props = {
            currentChatId: chatId,
            currentChatName: chatName,
            isMobile: true
        };
        if (mobileStack.depth === 1)
            mobileStack.push(chatViewComponent, props);
        else
            mobileStack.replace(chatViewComponent, props);
    }

    StackView {
        id: mobileStack
        anchors.fill: parent

        pushEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 150
            }
        }
        pushExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 150
            }
        }

        initialItem: ChatList {}
    }

    Component {
        id: chatViewComponent
        ChatView {
            onBackRequested: mobileStack.pop()
        }
    }

    ConnectionIndicator {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 16
    }

    Connections {
        target: appController
        function onDialogOpenedSuccessfully(dialogId, peerName) {
            showChat(dialogId, peerName);
        }
    }
}
