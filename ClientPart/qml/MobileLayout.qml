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

        initialItem: ChatList {
            onMenuRequested: drawer.open()
        }
    }

    Component {
        id: chatViewComponent
        ChatView {
            onBackRequested: mobileStack.pop()
        }
    }

    Component {
        id: settingsComponent
        SettingsPage {
            isMobile: true
            onBackRequested: mobileStack.pop()
        }
    }

    SideDrawer {
        id: drawer
        swipeEnabled: mobileStack.depth === 1
        onSettingsRequested: mobileStack.push(settingsComponent)
    }

    Connections {
        target: appController
        function onDialogOpenedSuccessfully(dialogId, peerName) {
            showChat(dialogId, peerName);
        }
    }
}
