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
}
