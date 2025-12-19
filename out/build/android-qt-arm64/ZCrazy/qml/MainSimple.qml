import QtQuick
import QtQuick.Window

Window {
    id: win
    visible: true
    width: 360
    height: 640
    color: "#202020"
    title: "ZCrazy Minimal"

    Text {
        anchors.centerIn: parent
        color: "white"
        text: "ZCrazy Minimal"
        font.pixelSize: 28
    }

    // Keep the activity alive; we don't want the app to auto-quit on Android
    onClosing: function(e) {
        // Prevent closing to avoid the activity finishing immediately
        e.accepted = false
    }

    Component.onCompleted: console.log("[QML] MainSimple.qml completed; minimal Window scene is running; visible=", visible)
}
