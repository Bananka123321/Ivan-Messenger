import QtQuick

Rectangle {
    id: root

    property string name: ""
    property int size: 40

    readonly property var colors: ["#3B82F6", "#10B981", "#F59E0B", "#EF4444", "#8B5CF6", "#EC4899", "#14B8A6", "#F97316"]

    implicitWidth: size
    implicitHeight: size
    radius: size / 2
    color: {
        let hash = 0;
        for (let i = 0; i < name.length; ++i)
            hash = (hash * 31 + name.charCodeAt(i)) | 0;
        return colors[Math.abs(hash) % colors.length];
    }

    Text {
        anchors.centerIn: parent
        text: root.name ? root.name.charAt(0).toUpperCase() : "?"
        color: "#FFFFFF"
        font.bold: true
        font.pixelSize: Math.round(root.size * 0.42)
    }
}
