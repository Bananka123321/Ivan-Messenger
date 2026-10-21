pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    visible: true
    width: 1000
    height: 700
    minimumWidth: 320
    minimumHeight: 500
    title: "Ivan Messenger"
    color: "#18181B"

    property bool shouldShowLogin: !appState.hasSession()

    StackView {
        id: authStackView
        anchors.fill: parent

        initialItem: shouldShowLogin ? loginScreenComponent : mainLayoutComponent
    }

    Component {
        id: loginScreenComponent
        LoginScreen {
            onLoginSucceeded: loadMainLayout()
            onSwitchToRegister: authStackView.replace(registerScreenComponent)
        }
    }

    Component {
        id: registerScreenComponent
        RegisterScreen {
            onRegisterSucceeded: loadMainLayout()
            onSwitchToLogin: authStackView.replace(loginScreenComponent)
        }
    }

    Component {
        id: mainLayoutComponent
        Item {
            Loader {
                anchors.fill: parent
                source: root.width < 600 ? "MobileLayout.qml" : "DesktopLayout.qml"
            }
        }
    }

    function loadMainLayout() {
        shouldShowLogin = false;
        authStackView.replace(mainLayoutComponent);
    }

    function todo(feature) {
        showToast(feature + ": скоро будет");
    }

    function showToast(text) {
        toastText.text = text;
        toast.open();
        toastTimer.restart();
    }

    function logout() {
        appState.clearSession();
        showLogin();
    }

    function showLogin() {
        shouldShowLogin = true;
        if (authStackView.currentItem !== loginScreenComponent) {
            authStackView.replace(loginScreenComponent);
        }
    }

    Component.onCompleted: {
        appController.checkAndResumeSession();
        console.log("AppState object:", appState);
        console.log("Connection status:", appState.connectionStatus);
    }

    Connections {
        target: appController
        function onResumeSessionFinished(success) {
            if (!success) {
                if (!shouldShowLogin)
                    showLogin();
                appState.clearSession();
            } else if (success && !shouldShowLogin) {
                if (authStackView.currentItem === loginScreenComponent) {
                    loadMainLayout();
                }
            }
        }
    }

    Popup {
        id: toast
        parent: Overlay.overlay
        x: (parent.width - width) / 2
        y: parent.height - height - 96
        width: Math.min(implicitWidth, parent.width - 32)
        padding: 12
        leftPadding: 16
        rightPadding: 16
        closePolicy: Popup.NoAutoClose

        background: Rectangle {
            color: "#3F3F46"
            radius: 10
        }

        contentItem: Text {
            id: toastText
            color: "#F4F4F5"
            font.pixelSize: 14
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }

        enter: Transition {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 150
            }
        }
        exit: Transition {
            NumberAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 150
            }
        }

        Timer {
            id: toastTimer
            interval: 2200
            onTriggered: toast.close()
        }
    }
}
