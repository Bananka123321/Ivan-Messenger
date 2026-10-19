import QtQuick
import QtQuick.Layouts

Item {
    RowLayout {
        anchors.fill: parent
        spacing: 0

        ChatList {
            Layout.preferredWidth: 320
            Layout.fillHeight: true
        }

        ChatView {
            id: chatView
            Layout.fillWidth: true
            Layout.fillHeight: true
            isMobile: false
        }

        ConnectionIndicator {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 16
        }
    }

    Connections {
        target: appController
        function onDialogOpenedSuccessfully(dialogId, peerName) {
            chatView.openChat(dialogId, peerName);
        }
    }
}
