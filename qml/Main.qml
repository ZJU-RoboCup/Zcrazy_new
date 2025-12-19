import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: win
    visible: true
    width: 1280; height: 600
    title: "ZCrazy Monitor"
    color: "#222"
    Component.onCompleted: console.log("[QML] Main.qml monitor mode ready")

    // 仅保留监控：左侧列表 + 右侧传感器详情
    Row {
        anchors.fill: parent
        spacing: 0

        // 在线车辆列表面板
        InfoViewer {
            id: viewer
            width: Math.min(420, Math.max(280, win.width * 0.33))
            height: parent.height
        }

        // 传感器详情面板（已内嵌于 InfoViewer，留出扩展空间）
        Rectangle {
            id: spacer
            width: win.width - viewer.width
            height: parent.height
            color: "#333"
            // 详情直接使用 viewer 的面板, 这里可以后续改为多车历史/图表
            // 暂留空白（仅视觉分割），所有详情现在在 InfoViewer 内部下半部分显示
        }
    }
}
