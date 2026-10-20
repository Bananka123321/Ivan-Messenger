import QtQuick
import QtQuick.Controls

AbstractButton {
    id: control

    property string glyph: ""
    property color glyphColor: "#A1A1AA"
    property int glyphSize: 20
    property color bgColor: "transparent"
    property color hoverColor: "#2C2C30"
    property color pressedColor: "#3F3F46"
    property string tip: ""

    implicitWidth: 40
    implicitHeight: 40
    hoverEnabled: true
    focusPolicy: Qt.NoFocus

    background: Rectangle {
        radius: width / 2
        color: control.pressed ? control.pressedColor : (control.hovered ? control.hoverColor : control.bgColor)
        Behavior on color {
            ColorAnimation {
                duration: 120
            }
        }
    }

    contentItem: Text {
        text: control.glyph
        color: control.enabled ? control.glyphColor : "#52525B"
        font.pixelSize: control.glyphSize
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    ToolTip.visible: tip !== "" && hovered
    ToolTip.delay: 600
    ToolTip.text: tip
}
