import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    id: root
    color: "#18181B"

    property int currentChatId: 0
    property string currentChatName: "Выберите чат"
    property bool isMobile: false
    property string peerStatus: ""

    signal backRequested

    function openChat(chatId, chatName) {
        currentChatName = chatName;
        currentChatId = chatId;
        composer.clear();
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

    function todo(feature) {
        ApplicationWindow.window.todo(feature);
    }

    function sameDay(a, b) {
        const da = new Date(a);
        const db = new Date(b);
        return da.getFullYear() === db.getFullYear() && da.getMonth() === db.getMonth() && da.getDate() === db.getDate();
    }

    function formatDate(ts) {
        const now = Date.now();
        if (sameDay(ts, now))
            return "Сегодня";
        if (sameDay(ts, now - 24 * 60 * 60 * 1000))
            return "Вчера";
        return new Date(ts).toLocaleDateString(Qt.locale("ru_RU"), "d MMMM yyyy");
    }

    function handleSend(text, attachments, replyToId, editMsgId) {
        if (currentChatId === 0)
            return;
        if (editMsgId) {
            todo("Редактирование сообщений");
            return;
        }
        if (attachments.length > 0) {
            todo("Отправка файлов");
            return;
        }
        if (replyToId) {
            todo("Ответы на сообщения");
            return;
        }
        appController.sendText(currentChatId, text);
        composer.clear();
    }

    function handleAttachment(kind) {
        switch (kind) {
        case "gallery":
            fileDialog.nameFilters = ["Фото и видео (*.png *.jpg *.jpeg *.gif *.webp *.heic *.mp4 *.mov *.mkv *.webm)", "Все файлы (*)"];
            fileDialog.open();
            break;
        case "file":
            fileDialog.nameFilters = ["Все файлы (*)"];
            fileDialog.open();
            break;
        case "camera":
            todo("Съёмка с камеры");
            break;
        case "location":
            todo("Отправка геопозиции");
            break;
        case "contact":
            todo("Отправка контакта");
            break;
        case "poll":
            todo("Опросы");
            break;
        }
    }

    function copyToClipboard(text) {
        clipboardHelper.text = text;
        clipboardHelper.selectAll();
        clipboardHelper.copy();
        ApplicationWindow.window.showToast("Скопировано");
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
        visible: root.currentChatId === 0
        anchors.centerIn: parent
        spacing: 12

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "💬"
            font.pixelSize: 56
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Выберите чат, чтобы начать общение"
            color: "#A1A1AA"
            font.pixelSize: 16
        }
    }

    ColumnLayout {
        visible: root.currentChatId !== 0
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#27272A"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: root.isMobile ? 4 : 16
                anchors.rightMargin: 8
                spacing: 8

                IconButton {
                    visible: root.isMobile
                    glyph: "←"
                    glyphSize: 22
                    glyphColor: "#F4F4F5"
                    onClicked: root.backRequested()
                }

                AbstractButton {
                    id: peerHeader
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    contentItem: RowLayout {
                        spacing: 12

                        Avatar {
                            name: root.currentChatName
                            size: 40
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: root.currentChatName
                                color: "#F4F4F5"
                                font.bold: true
                                font.pixelSize: 17
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Text {
                                visible: root.peerStatus !== ""
                                text: root.peerStatus
                                color: "#71717A"
                                font.pixelSize: 13
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                        }
                    }

                    onClicked: root.todo("Профиль собеседника")
                }

                IconButton {
                    glyph: "📞"
                    tip: "Аудиозвонок"
                    onClicked: root.todo("Аудиозвонки")
                }

                IconButton {
                    visible: !root.isMobile
                    glyph: "📹"
                    tip: "Видеозвонок"
                    onClicked: root.todo("Видеозвонки")
                }

                IconButton {
                    id: moreButton
                    glyph: "⋮"
                    glyphSize: 22
                    tip: "Ещё"
                    onClicked: chatMenu.popup(moreButton, 0, moreButton.height)
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: messagesView
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                topMargin: 12
                bottomMargin: 12
                spacing: 6
                clip: true
                model: ListModel {
                    id: msgModel
                }
                verticalLayoutDirection: ListView.BottomToTop

                delegate: Column {
                    id: row
                    required property int index
                    required property var msgId
                    required property string messageText
                    required property bool isMine
                    required property var timestamp

                    readonly property bool showDate: index + 1 >= msgModel.count || !root.sameDay(msgModel.get(index + 1).timestamp, timestamp)

                    width: messagesView.width
                    spacing: 6

                    Item {
                        visible: row.showDate
                        width: parent.width
                        height: 32

                        Rectangle {
                            anchors.centerIn: parent
                            width: dateText.implicitWidth + 20
                            height: 24
                            radius: 12
                            color: "#27272A"

                            Text {
                                id: dateText
                                anchors.centerIn: parent
                                text: root.formatDate(row.timestamp)
                                color: "#A1A1AA"
                                font.pixelSize: 12
                            }
                        }
                    }

                    Rectangle {
                        id: bubble
                        readonly property real maxWidth: Math.min(messagesView.width * 0.78, 520)

                        x: row.isMine ? parent.width - width : 0
                        width: Math.max(msgText.width, meta.implicitWidth) + 24
                        height: bubbleColumn.implicitHeight + 16
                        radius: 14
                        color: bubbleMouse.pressed ? (row.isMine ? "#2563EB" : "#3F3F46") : (row.isMine ? "#3B82F6" : "#27272A")

                        Column {
                            id: bubbleColumn
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 12
                            anchors.topMargin: 8
                            spacing: 2

                            Text {
                                id: msgText
                                text: row.messageText
                                textFormat: Text.PlainText
                                color: "#F4F4F5"
                                font.pixelSize: 15
                                wrapMode: Text.Wrap
                                width: Math.min(implicitWidth, bubble.maxWidth - 24)
                            }

                            Text {
                                id: meta
                                anchors.right: parent.right
                                text: Qt.formatTime(new Date(row.timestamp), "hh:mm") + (row.isMine ? "  ✓" : "")
                                color: row.isMine ? "#DBEAFE" : "#71717A"
                                font.pixelSize: 11
                            }
                        }

                        MouseArea {
                            id: bubbleMouse
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onPressAndHold: mouse => root.openMessageMenu(row, bubble, mouse.x, mouse.y)
                            onClicked: mouse => {
                                if (mouse.button === Qt.RightButton)
                                    root.openMessageMenu(row, bubble, mouse.x, mouse.y);
                            }
                        }
                    }
                }
            }

            Text {
                visible: msgModel.count === 0
                anchors.centerIn: parent
                horizontalAlignment: Text.AlignHCenter
                text: "Сообщений пока нет\nНапишите первым 👋"
                color: "#71717A"
                font.pixelSize: 15
            }

            IconButton {
                visible: !messagesView.atYEnd && msgModel.count > 0
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 12
                implicitWidth: 44
                implicitHeight: 44
                glyph: "↓"
                glyphColor: "#F4F4F5"
                bgColor: "#3F3F46"
                hoverColor: "#52525B"
                pressedColor: "#71717A"
                tip: "К последним сообщениям"
                onClicked: messagesView.positionViewAtBeginning()
            }

            DropArea {
                id: dropArea
                anchors.fill: parent
                onDropped: drop => {
                    if (drop.hasUrls)
                        composer.addAttachments(drop.urls);
                }
            }

            Rectangle {
                visible: dropArea.containsDrag
                anchors.fill: parent
                anchors.margins: 12
                radius: 16
                color: "#CC18181B"
                border.color: "#3B82F6"
                border.width: 2

                Text {
                    anchors.centerIn: parent
                    text: "Отпустите, чтобы прикрепить"
                    color: "#F4F4F5"
                    font.pixelSize: 16
                }
            }
        }

        MessageComposer {
            id: composer
            Layout.fillWidth: true
            isMobile: root.isMobile
            onSendRequested: (text, attachments, replyToId, editMsgId) => root.handleSend(text, attachments, replyToId, editMsgId)
            onAttachRequested: anchorItem => attachmentSheet.openAt(anchorItem)
            onEmojiRequested: anchorItem => emojiPanel.openAt(anchorItem)
            onVoiceRequested: root.todo("Голосовые сообщения")
        }
    }

    AttachmentSheet {
        id: attachmentSheet
        isMobile: root.isMobile
        onPicked: kind => root.handleAttachment(kind)
    }

    EmojiPanel {
        id: emojiPanel
        isMobile: root.isMobile
        onEmojiPicked: emoji => composer.insertText(emoji)
        onStickersRequested: root.todo("Стикеры")
        onGifRequested: root.todo("GIF")
    }

    FileDialog {
        id: fileDialog
        title: "Выберите файлы"
        fileMode: FileDialog.OpenFiles
        onAccepted: composer.addAttachments(selectedFiles)
    }

    AppMenu {
        id: chatMenu

        AppMenuItem {
            glyph: "🔍"
            text: "Поиск по чату"
            onTriggered: root.todo("Поиск по чату")
        }
        AppMenuItem {
            glyph: "🗂"
            text: "Медиа и файлы"
            onTriggered: root.todo("Медиа и файлы чата")
        }
        AppMenuItem {
            visible: root.isMobile
            glyph: "📹"
            text: "Видеозвонок"
            onTriggered: root.todo("Видеозвонки")
        }
        AppMenuItem {
            glyph: "🔕"
            text: "Без звука"
            onTriggered: root.todo("Отключение уведомлений")
        }
        MenuSeparator {
            contentItem: Rectangle {
                implicitHeight: 1
                color: "#3F3F46"
            }
        }
        AppMenuItem {
            glyph: "🧹"
            text: "Очистить историю"
            onTriggered: {
                confirmDialog.title = "Очистить историю?";
                confirmDialog.text = "Сообщения в этом чате будут удалены.";
                confirmDialog.confirmText = "Очистить";
                confirmDialog.action = "clear";
                confirmDialog.open();
            }
        }
        AppMenuItem {
            glyph: "⛔"
            text: "Заблокировать"
            destructive: true
            onTriggered: {
                confirmDialog.title = "Заблокировать " + root.currentChatName + "?";
                confirmDialog.text = "Пользователь не сможет писать вам.";
                confirmDialog.confirmText = "Заблокировать";
                confirmDialog.action = "block";
                confirmDialog.open();
            }
        }
        AppMenuItem {
            glyph: "🗑"
            text: "Удалить чат"
            destructive: true
            onTriggered: {
                confirmDialog.title = "Удалить чат?";
                confirmDialog.text = "Чат с " + root.currentChatName + " будет удалён.";
                confirmDialog.confirmText = "Удалить";
                confirmDialog.action = "delete";
                confirmDialog.open();
            }
        }
    }

    ConfirmDialog {
        id: confirmDialog
        property string action: ""
        destructive: true
        onConfirmed: {
            switch (action) {
            case "clear":
                root.todo("Очистка истории");
                break;
            case "block":
                root.todo("Блокировка пользователей");
                break;
            case "delete":
                root.todo("Удаление чатов");
                break;
            case "deleteMessage":
                root.todo("Удаление сообщений");
                break;
            }
        }
    }

    property var menuMsgId: 0
    property string menuMsgText: ""
    property bool menuMsgMine: false

    function openMessageMenu(row, bubble, x, y) {
        menuMsgId = row.msgId;
        menuMsgText = row.messageText;
        menuMsgMine = row.isMine;
        messageMenu.popup(bubble, x, y);
    }

    AppMenu {
        id: messageMenu

        AppMenuItem {
            glyph: "↩"
            text: "Ответить"
            onTriggered: composer.startReply(root.menuMsgId, root.menuMsgText)
        }
        AppMenuItem {
            glyph: "📋"
            text: "Копировать"
            onTriggered: root.copyToClipboard(root.menuMsgText)
        }
        AppMenuItem {
            visible: root.menuMsgMine
            glyph: "✎"
            text: "Изменить"
            onTriggered: composer.startEdit(root.menuMsgId, root.menuMsgText)
        }
        AppMenuItem {
            glyph: "↪"
            text: "Переслать"
            onTriggered: root.todo("Пересылка сообщений")
        }
        AppMenuItem {
            glyph: "📌"
            text: "Закрепить"
            onTriggered: root.todo("Закреплённые сообщения")
        }
        AppMenuItem {
            glyph: "☑"
            text: "Выбрать"
            onTriggered: root.todo("Выбор нескольких сообщений")
        }
        AppMenuItem {
            glyph: "🗑"
            text: "Удалить"
            destructive: true
            onTriggered: {
                confirmDialog.title = "Удалить сообщение?";
                confirmDialog.text = "";
                confirmDialog.confirmText = "Удалить";
                confirmDialog.action = "deleteMessage";
                confirmDialog.open();
            }
        }
    }

    TextEdit {
        id: clipboardHelper
        visible: false
    }
}
