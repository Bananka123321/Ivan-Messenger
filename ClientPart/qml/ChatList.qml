import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ClientModule

Rectangle {
    id: chatListRoot
    color: "#202023"

    property bool isSearching: false

    signal menuRequested

    function startSearch() {
        isSearching = true;
        searchField.forceActiveFocus();
    }

    function stopSearch() {
        isSearching = false;
        searchField.text = "";
        if (searchViewModel)
            searchViewModel.searchUser("");
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
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 4

                IconButton {
                    visible: !chatListRoot.isSearching
                    glyph: "☰"
                    glyphSize: 22
                    glyphColor: "#F4F4F5"
                    tip: "Меню"
                    onClicked: chatListRoot.menuRequested()
                }

                Text {
                    visible: !chatListRoot.isSearching
                    text: "Чаты"
                    color: "#F4F4F5"
                    font.bold: true
                    font.pixelSize: 20
                    Layout.fillWidth: true
                }

                ConnectionIndicator {
                    visible: !chatListRoot.isSearching
                }

                IconButton {
                    visible: !chatListRoot.isSearching
                    glyph: "🔍"
                    tip: "Поиск"
                    onClicked: chatListRoot.startSearch()
                }

                TextField {
                    id: searchField
                    visible: chatListRoot.isSearching
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    placeholderText: "Поиск..."
                    placeholderTextColor: "#71717A"
                    color: "#F4F4F5"
                    font.pixelSize: 16

                    background: Rectangle {
                        color: "#18181B"
                        radius: 8
                    }

                    onTextChanged: {
                        searchTimer.restart();
                    }

                    Timer {
                        id: searchTimer
                        interval: 300
                        onTriggered: {
                            if (searchViewModel) {
                                searchViewModel.searchUser(searchField.text);
                            }
                        }
                    }
                }

                IconButton {
                    visible: chatListRoot.isSearching
                    glyph: "✕"
                    glyphSize: 20
                    tip: "Закрыть поиск"
                    onClicked: chatListRoot.stopSearch()
                }
            }
        }

        ListView {
            id: dialogsView
            visible: !chatListRoot.isSearching
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: dialogManager ? dialogManager.dialogs : []
            clip: true

            delegate: Rectangle {
                width: parent.width
                height: 70
                color: mouseArea.containsMouse || mouseArea.pressed ? "#2C2C30" : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    Avatar {
                        name: modelData.name ? modelData.name : ""
                        size: 48
                        Layout.alignment: Qt.AlignVCenter
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 4
                        Text {
                            text: modelData.name
                            color: "#F4F4F5"
                            font.bold: true
                            font.pixelSize: 16
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        Text {
                            text: modelData.lastMsg
                            color: "#A1A1AA"
                            font.pixelSize: 14
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }

                    ColumnLayout {
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 6

                        Text {
                            text: modelData.time
                            color: "#71717A"
                            font.pixelSize: 12
                            Layout.alignment: Qt.AlignRight
                        }

                        Rectangle {
                            visible: (modelData.unread || 0) > 0
                            Layout.alignment: Qt.AlignRight
                            Layout.preferredWidth: Math.max(20, unreadText.implicitWidth + 10)
                            Layout.preferredHeight: 20
                            radius: 10
                            color: "#3B82F6"

                            Text {
                                id: unreadText
                                anchors.centerIn: parent
                                text: modelData.unread || ""
                                color: "#FFFFFF"
                                font.bold: true
                                font.pixelSize: 11
                            }
                        }
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (appController)
                            appController.openDialog(modelData.peerId);
                    }
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 1
                    color: "#2C2C30"
                }
            }
        }

        Rectangle {
            visible: chatListRoot.isSearching
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#202023"

            Rectangle {
                visible: searchViewModel ? searchViewModel.isSearching : false
                anchors.fill: parent
                color: "#202023"
                Text {
                    anchors.centerIn: parent
                    text: "Поиск..."
                    color: "#A1A1AA"
                    font.pixelSize: 14
                }
            }

            ListView {
                visible: !(searchViewModel ? searchViewModel.isSearching : false) && (searchViewModel ? searchViewModel.users.length > 0 : false)
                anchors.fill: parent
                model: searchViewModel ? searchViewModel.users : []
                clip: true

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 60
                    color: mouseArea.containsMouse ? "#2C2C30" : "#202023"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 12

                        Avatar {
                            name: modelData.username ? modelData.username : ""
                            size: 40
                        }

                        Text {
                            text: modelData.username ? modelData.username : "Unknown"
                            color: "#F4F4F5"
                            font.pixelSize: 16
                            Layout.fillWidth: true
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            const userId = modelData.id;
                            if (searchViewModel)
                                searchViewModel.selectUser(userId);

                            chatListRoot.isSearching = false;
                            searchField.text = "";
                            if (searchViewModel)
                                searchViewModel.searchUser("");
                        }
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 1
                        color: "#2C2C30"
                    }
                }
            }

            Text {
                visible: !(searchViewModel ? searchViewModel.isSearching : false) && (searchField.text.length > 0) && !(searchViewModel ? searchViewModel.users.length > 0 : false)
                anchors.centerIn: parent
                text: "Пользователи не найдены"
                color: "#71717A"
                font.pixelSize: 14
            }
        }
    }

    ColumnLayout {
        visible: !chatListRoot.isSearching && dialogsView.count === 0
        anchors.centerIn: parent
        spacing: 8

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Чатов пока нет"
            color: "#A1A1AA"
            font.pixelSize: 16
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Найдите собеседника через поиск"
            color: "#71717A"
            font.pixelSize: 14
        }
    }

    IconButton {
        id: newChatButton
        visible: !chatListRoot.isSearching
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        implicitWidth: 56
        implicitHeight: 56
        glyph: "✎"
        glyphSize: 24
        glyphColor: "#FFFFFF"
        bgColor: "#3B82F6"
        hoverColor: "#2563EB"
        pressedColor: "#1D4ED8"
        tip: "Новый чат"
        onClicked: newChatMenu.popup(newChatButton, newChatButton.width - newChatMenu.implicitWidth, -newChatMenu.implicitHeight - 8)
    }

    AppMenu {
        id: newChatMenu

        AppMenuItem {
            glyph: "👤"
            text: "Новый чат"
            onTriggered: chatListRoot.startSearch()
        }
        AppMenuItem {
            glyph: "👥"
            text: "Новая группа"
            onTriggered: ApplicationWindow.window.todo("Группы")
        }
        AppMenuItem {
            glyph: "📢"
            text: "Новый канал"
            onTriggered: ApplicationWindow.window.todo("Каналы")
        }
    }
}
