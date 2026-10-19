import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ClientModule

Rectangle {
    id: chatListRoot
    color: "#202023"

    property bool isSearching: false

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

                Text {
                    visible: !chatListRoot.isSearching
                    text: "Чаты"
                    color: "#F4F4F5"
                    font.bold: true
                    font.pixelSize: 20
                    Layout.fillWidth: true
                }

                Button {
                    visible: !chatListRoot.isSearching
                    text: "🔍"
                    font.pixelSize: 22
                    background: Rectangle {
                        color: "transparent"
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#A1A1AA"
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        chatListRoot.isSearching = true;
                        searchField.forceActiveFocus();
                    }
                }

                TextField {
                    id: searchField
                    visible: chatListRoot.isSearching
                    Layout.fillWidth: true
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

                Button {
                    visible: chatListRoot.isSearching
                    text: "✕"
                    font.pixelSize: 24
                    font.bold: true
                    background: Rectangle {
                        color: "transparent"
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#A1A1AA"
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        chatListRoot.isSearching = false;
                        searchField.text = "";
                        if (searchViewModel)
                            searchViewModel.searchUser("");
                    }
                }
            }
        }

        ListView {
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

                    Rectangle {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        radius: 24
                        color: "#3B82F6"
                        Layout.alignment: Qt.AlignVCenter
                        Text {
                            anchors.centerIn: parent
                            text: modelData.name ? modelData.name.charAt(0).toUpperCase() : "?"
                            color: "#FFFFFF"
                            font.bold: true
                            font.pixelSize: 20
                        }
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

                    Text {
                        text: modelData.time
                        color: "#71717A"
                        font.pixelSize: 12
                        Layout.alignment: Qt.AlignTop | Qt.AlignRight
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

                        Rectangle {
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            radius: 20
                            color: "#10B981"
                            Text {
                                anchors.centerIn: parent
                                text: modelData.username ? modelData.username.charAt(0).toUpperCase() : "?"
                                color: "#FFFFFF"
                                font.bold: true
                                font.pixelSize: 18
                            }
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
}
