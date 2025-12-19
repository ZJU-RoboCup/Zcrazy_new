import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 1280
    height: 600
    title: "ZCrazy"

    property bool needChangeTeamTickEnable: false

    Timer{
        id:timer;
        interval:8;
        running:false;
        repeat:true;
        onTriggered: {
            infoViewer.sendCommand();//把数据发出去
        }
    }

    Timer {
        id: resetTimer
        interval: 1000  // 1秒自动恢复
        repeat: false
        onTriggered: needChangeTeamTickEnable = false
    }

    onClosing: {
        // no op
    }
    Rectangle{
        width:parent.width-infoViewerRect.width
        height:parent.height
        anchors.left:parent.left
        color:"#222"

        focus: true
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_T) {
                needChangeTeamTickEnable = true
                resetTimer.restart()
                event.accepted = true
            }
        }
        UI{
            needChangeTeamState : needChangeTeamTickEnable
            cmdSender:infoViewer
        }
    }
    Rectangle{
        id:infoViewerRect
        width:500
        height:parent.height
        anchors.right:parent.right
        color:"#444"

        // Stub that forwards to C++ command client
        Item{
            id: infoViewer
            anchors.fill: parent
            property bool needChangeTeam: needChangeTeamTickEnable
            onNeedChangeTeamChanged: commandClient.setNeedChangeTeam(needChangeTeam)

            function updateCommandParams(robotID,velX,velY,velR,ctrl,mode,shoot,power,use_imu,angle,control_all,control_all_which_team){
                commandClient.updateCommandParams(robotID,velX,velY,velR,ctrl,mode,shoot,power,use_imu,angle,control_all,control_all_which_team)
            }
            function sendCommand(){ commandClient.sendCommand() }
            function car_num(only_one) { /* no-op */ }
            function plotStart() { /* no-op */ }
            function plotStop() { /* no-op */ }
        }
    }

}
