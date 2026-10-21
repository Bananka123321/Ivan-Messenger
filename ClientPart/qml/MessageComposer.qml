import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: composer

    property bool isMobile: false

    property string mode: ""
    property var targetMsgId: 0
    property string targetPreview: ""

    readonly property bool hasContent: input.text.trim() !== "" || attachModel.count > 0

    signal sendRequested(string text, var attachments, var replyToId, var editMsgId)
    signal attachRequested(Item anchorItem)
    signal emojiRequested(Item anchorItem)
    signal voiceRequested

    function startReply(msgId, preview) {
        mode = "reply";
        targetMsgId = msgId;
        targetPreview = preview;
        input.forceActiveFocus();
    }

    function startEdit(msgId, text) {
        mode = "edit";
        targetMsgId = msgId;
        targetPreview = text;
        input.text = text;
        input.forceActiveFocus();
        input.cursorPosition = input.length;
    }

    function cancelMode() {
        if (mode === "edit")
            input.text = "";
        mode = "";
        targetMsgId = 0;
        targetPreview = "";
    }

    function insertText(text) {
        input.insert(input.cursorPosition, text);
        if (!isMobile)
            input.forceActiveFocus();
    }

    function kindForUrl(url) {
        const ext = url.toString().split(".").pop().toLowerCase();
        if (["png", "jpg", "jpeg", "gif", "webp", "bmp", "heic"].indexOf(ext) !== -1)
            return "image";
        if (["mp4", "mov", "mkv", "webm", "avi", "3gp"].indexOf(ext) !== -1)
            return "video";
        return "file";
    }

    function addAttachments(urls) {
        for (let i = 0; i < urls.length; ++i) {
            const url = urls[i].toString();
            const name = decodeURIComponent(url.split("/").pop());
            attachModel.append({
                url: url,
                name: name,
                kind: kindForUrl(url)
            });
        }
    }

    function clear() {
        input.text = "";
        attachModel.clear();
        mode = "";
        targetMsgId = 0;
        targetPreview = "";
    }

    function handleReturn(event) {
        if (!isMobile && !(event.modifiers & Qt.ShiftModifier)) {
            send();
            event.accepted = true;
        } else {
            event.accepted = false;
        }
    }

    function send() {
        if (!hasContent)
            return;
        const attachments = [];
        for (let i = 0; i < attachModel.count; ++i) {
            const a = attachModel.get(i);
            attachments.push({
                url: a.url,
                name: a.name,
                kind: a.kind
            });
        }
        sendRequested(input.text.trim(), attachments, mode === "reply" ? targetMsgId : 0, mode === "edit" ? targetMsgId : 0);
    }

    implicitHeight: column.implicitHeight + 16
    color: "#27272A"

    ListModel {
        id: attachModel
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        RowLayout {
            visible: composer.mode !== ""
            Layout.fillWidth: true
            Layout.leftMargin: 4
            spacing: 10

            Text {
                text: composer.mode === "edit" ? "✎" : "↩"
                color: "#3B82F6"
                font.pixelSize: 20
            }

            Rectangle {
                Layout.preferredWidth: 2
                Layout.fillHeight: true
                color: "#3B82F6"
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: composer.mode === "edit" ? "Редактирование" : "Ответ на сообщение"
                    color: "#3B82F6"
                    font.bold: true
                    font.pixelSize: 13
                }
                Text {
                    text: composer.targetPreview
                    textFormat: Text.PlainText
                    color: "#A1A1AA"
                    font.pixelSize: 13
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            IconButton {
                glyph: "✕"
                glyphSize: 16
                implicitWidth: 32
                implicitHeight: 32
                tip: "Отменить"
                onClicked: composer.cancelMode()
            }
        }

        ListView {
            visible: attachModel.count > 0
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            orientation: ListView.Horizontal
            spacing: 8
            clip: true
            model: attachModel

            delegate: Rectangle {
                id: chip
                required property int index
                required property string name
                required property string kind

                width: Math.min(200, chipRow.implicitWidth + 16)
                height: 48
                radius: 10
                color: "#18181B"
                border.color: "#3F3F46"
                border.width: 1

                RowLayout {
                    id: chipRow
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 4
                    spacing: 8

                    Text {
                        text: chip.kind === "image" ? "🖼" : (chip.kind === "video" ? "🎬" : "📄")
                        font.pixelSize: 20
                    }
                    Text {
                        text: chip.name
                        textFormat: Text.PlainText
                        color: "#E4E4E7"
                        font.pixelSize: 13
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                        Layout.maximumWidth: 120
                    }
                    IconButton {
                        glyph: "✕"
                        glyphSize: 13
                        implicitWidth: 28
                        implicitHeight: 28
                        onClicked: attachModel.remove(chip.index)
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            IconButton {
                id: attachButton
                Layout.alignment: Qt.AlignBottom
                implicitWidth: 44
                implicitHeight: 44
                glyph: "📎"
                glyphSize: 22
                tip: "Прикрепить"
                onClicked: composer.attachRequested(attachButton)
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(Math.max(44, input.implicitHeight), 140)
                radius: 22
                color: "#18181B"
                border.color: input.activeFocus ? "#3B82F6" : "#3F3F46"
                border.width: 1

                Flickable {
                    id: flick
                    anchors.left: parent.left
                    anchors.right: emojiButton.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 14
                    clip: true
                    contentWidth: width
                    contentHeight: input.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds

                    TextArea.flickable: TextArea {
                        id: input
                        placeholderText: "Сообщение..."
                        placeholderTextColor: "#71717A"
                        color: "#F4F4F5"
                        font.pixelSize: 15
                        wrapMode: TextArea.Wrap
                        leftPadding: 0
                        rightPadding: 0
                        topPadding: 11
                        bottomPadding: 11
                        background: null

                        Keys.onReturnPressed: event => composer.handleReturn(event)
                        Keys.onEnterPressed: event => composer.handleReturn(event)
                        Keys.onEscapePressed: composer.cancelMode()
                    }
                }

                IconButton {
                    id: emojiButton
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: 2
                    anchors.bottomMargin: 2
                    glyph: "😊"
                    tip: "Эмодзи"
                    onClicked: composer.emojiRequested(emojiButton)
                }
            }

            IconButton {
                id: sendButton
                Layout.alignment: Qt.AlignBottom
                implicitWidth: 44
                implicitHeight: 44
                glyph: composer.hasContent ? (composer.mode === "edit" ? "✓" : "➤") : "🎤"
                glyphColor: "#FFFFFF"
                glyphSize: composer.hasContent ? 20 : 18
                bgColor: "#3B82F6"
                hoverColor: "#2563EB"
                pressedColor: "#1D4ED8"
                tip: composer.hasContent ? "Отправить" : "Голосовое сообщение"
                onClicked: {
                    if (composer.hasContent)
                        composer.send();
                    else
                        composer.voiceRequested();
                }
            }
        }
    }
}
