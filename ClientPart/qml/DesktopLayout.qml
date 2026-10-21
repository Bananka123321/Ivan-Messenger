import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    RowLayout {
        anchors.fill: parent
        spacing: 0

        ChatList {
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            onMenuRequested: drawer.open()
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: "#2C2C30"
        }

        ChatView {
            id: chatView
            Layout.fillWidth: true
            Layout.fillHeight: true
            isMobile: false
        }
    }

    SideDrawer {
        id: drawer
        onSettingsRequested: settingsPopup.open()
    }

    Popup {
        id: settingsPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 480)
        height: Math.min(parent.height - 32, 640)
        modal: true
        padding: 0

        background: Rectangle {
            color: "#18181B"
            radius: 12
            border.color: "#3F3F46"
            border.width: 1
        }

        SettingsPage {
            anchors.fill: parent
            anchors.margins: 1
            isMobile: false
            onBackRequested: settingsPopup.close()
        }
    }

    Connections {
        target: appController
        function onDialogOpenedSuccessfully(dialogId, peerName) {
            chatView.openChat(dialogId, peerName);
        }
    }
}
