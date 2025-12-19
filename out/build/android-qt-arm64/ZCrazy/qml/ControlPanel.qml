import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: 360
    property int robotID: 100 // treat as last IP octet for now
    property double velX: 0
    property double velY: 0
    property double velR: 0
    property double ctrl: 0
    property bool mode: false
    property bool shoot: false
    property double power: 0
    property bool useImu: false
    property double angle: 0
    property bool controlAll: false
    property bool controlAllWhichTeam: false

    signal paramsChanged(var params)

    function emitParams(){
        paramsChanged({
            robotID, velX, velY, velR,
            ctrl, mode, shoot, power,
            useImu, angle, controlAll, controlAllWhichTeam
        })
    }

    Column {
        spacing: 8
        Row {
            spacing: 8
            Text { text: "Robot Octet"; width: 100 }
            SpinBox { from: 1; to: 254; value: root.robotID; onValueModified: { root.robotID = value; root.emitParams(); } }
        }
        Row {
            spacing: 8
            Text { text: "Vx"; width: 100 }
            SpinBox { from: -1000; to: 1000; value: root.velX; onValueModified: { root.velX = value; root.emitParams(); } }
        }
        Row {
            spacing: 8
            Text { text: "Vy"; width: 100 }
            SpinBox { from: -1000; to: 1000; value: root.velY; onValueModified: { root.velY = value; root.emitParams(); } }
        }
        Row {
            spacing: 8
            Text { text: "Vr"; width: 100 }
            SpinBox { from: -1000; to: 1000; value: root.velR; onValueModified: { root.velR = value; root.emitParams(); } }
        }
        Row {
            spacing: 8
            Text { text: "Dribble"; width: 100 }
            SpinBox { from: 0; to: 30; value: root.ctrl; onValueModified: { root.ctrl = value; root.emitParams(); } }
        }
        Row {
            spacing: 8
            Text { text: "Shoot"; width: 100 }
            Switch { checked: root.shoot; onToggled: { root.shoot = checked; root.emitParams(); } }
        }
        Row {
            spacing: 8
            Text { text: "Mode (chip)"; width: 100 }
            Switch { checked: root.mode; onToggled: { root.mode = checked; root.emitParams(); } }
        }
        Row {
            spacing: 8
            Text { text: "Power"; width: 100 }
            SpinBox { from: 0; to: 200; value: root.power; onValueModified: { root.power = value; root.emitParams(); } }
        }
        Row {
            spacing: 8
            Text { text: "use IMU"; width: 100 }
            Switch { checked: root.useImu; onToggled: { root.useImu = checked; root.emitParams(); } }
        }
        Row {
            spacing: 8
            Text { text: "Angle"; width: 100 }
            SpinBox { from: -180; to: 180; value: root.angle; onValueModified: { root.angle = value; root.emitParams(); } }
        }
        Row {
            spacing: 8
            Text { text: "Control All"; width: 100 }
            Switch { checked: root.controlAll; onToggled: { root.controlAll = checked; root.emitParams(); } }
        }
        Row {
            spacing: 8
            Text { text: "Team (yellow)"; width: 100 }
            Switch { checked: root.controlAllWhichTeam; onToggled: { root.controlAllWhichTeam = checked; root.emitParams(); } }
        }
    }

    Component.onCompleted: emitParams()
}
