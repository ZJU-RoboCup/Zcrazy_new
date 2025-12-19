import QtQuick
import QtQuick.Controls

// 监控版 InfoViewer：仅显示在线车辆 + 选中车辆的详细传感器，不含任意控制命令。
Item {
    id: root
    anchors.fill: parent
    clip: true

    /* 选中状态 */
    property int selectedTeam: 0
    property int selectedRobot: -1
    // 只有点击 Start 后，点车辆才会触发单播订阅（详细面板）
    property bool started: false
    signal carSelected(int robotId)
    // 订阅状态跟踪（放在根作用域，便于 delegate 访问）
    property bool subscribedForCurrent: false
    property int subscribeAttempts: 0
    // 手工回退订阅 IP 列表：用于某些设备组播不可达时强制直连（示例包含 .94）
    // 如需关闭，设置为 []；如需添加，填入字符串 IP。
    property var manualSubscribeIps: []

    Component.onCompleted: {
        if (typeof robotStatusModel !== 'undefined') {
            robotStatusModel.detailEnabled = root.started
        }
    }

    // 轮询订阅：确保在 ip 延迟到达时也能触发订阅
    Timer {
        id: subscribePoll
        interval: 500
        repeat: true
        running: false
        onTriggered: {
            if (!root.started) { subscribePoll.stop(); return }
            if (root.subscribedForCurrent) { subscribePoll.stop(); return }
            if (!(root.selectedTeam && root.selectedRobot>=0)) { return }
            var info = (typeof robotStatusModel!=='undefined') ? robotStatusModel.getDetails(root.selectedTeam, root.selectedRobot) : ({});
            var ipCand = (info && info.ip) ? info.ip : (commandClient ? (commandClient.ipPrefix + "." + root.selectedRobot) : "");
            var did = false;
            if (ipCand) {
                console.log("[QML] subscribePoll subscribe", ipCand, root.selectedRobot)
                try { commandClient.subscribeWithIp(ipCand, root.selectedRobot); did = true; } catch(e) { /* ignore */ }
            }
            // 回退：对手工 IP 逐一尝试（尤其适用于 robot_id 与末段不一致的设备，如 id=0 但实际 IP=.94）
            if (root.manualSubscribeIps && root.manualSubscribeIps.length>0) {
                for (var i=0;i<root.manualSubscribeIps.length;i++){
                    var mip = root.manualSubscribeIps[i];
                    if (!mip) continue;
                    console.log("[QML] subscribePoll manual subscribe", mip, root.selectedRobot)
                    try { commandClient.subscribeWithIp(mip, root.selectedRobot); did = true; } catch(e) { /* ignore */ }
                }
            }
            if (did) {
                root.subscribedForCurrent = true;
                subscribePoll.stop();
            } else {
                root.subscribeAttempts += 1
                console.log("[QML] subscribePoll waiting ip attempt", root.subscribeAttempts)
                if (root.subscribeAttempts >= 8) { console.log("[QML] subscribePoll give up"); subscribePoll.stop() }
            }
        }
    }

    // 启动扫描：当列表为空且可能无法收到组播时，主动对 0..15 号尝试订阅，促使小车向手机单播回报
    Timer {
        id: startupScan
        interval: 400
        repeat: true
        running: false
        property int probeId: 0
        property int rounds: 0
        onTriggered: {
            // 按需求：必须 Start 后再点车才订阅单播；这里禁用自动扫描
            startupScan.stop();
            return;
            // 列表已有数据或当前有选中并已订阅，就停止扫描
            var hasList = (typeof robotStatusModel!=='undefined') ? (robotStatusModel.rowCount && robotStatusModel.rowCount()>0) : (list.count>0)
            if (hasList || root.subscribedForCurrent) { startupScan.stop(); return }
            if (!commandClient || !commandClient.ipPrefix) return
            // 逐个尝试 0..15 号（可根据需要扩展）
            var rid = startupScan.probeId
            var ipCand = commandClient.ipPrefix + "." + rid
            try { commandClient.subscribeWithIp(ipCand, rid) } catch(e) {}
            startupScan.probeId = (startupScan.probeId + 1) % 16
            startupScan.rounds += 1
            // 扫描若超过若干轮仍无反馈，则暂停，避免过度打扰
            if (startupScan.rounds >= 80) { startupScan.stop() }
        }
    }

    // 自动选中第一辆在线车（如果用户未手动选择）以触发订阅链路
    Timer {
        id: autoSelectFirst
        interval: 1200
        repeat: true
        running: false
        onTriggered: {
            if (root.selectedRobot >= 0) { autoSelectFirst.stop(); return }
            if (typeof robotStatusModel === 'undefined') return
            var count = robotStatusModel.rowCount ? robotStatusModel.rowCount() : (list.count)
            if (count && count > 0) {
                var item = (list.model && list.model.get) ? list.model.get(0) : null
                if (item && item.team && item.robotId !== undefined) {
                    root.selectedTeam = item.team; root.selectedRobot = item.robotId; root.carSelected(item.robotId)
                    console.log("[QML] autoSelect", root.selectedTeam, root.selectedRobot)
                    root.subscribedForCurrent = false; root.subscribeAttempts = 0; subscribePoll.stop()
                    autoSelectFirst.stop()
                }
            }
        }
    }
    // 订阅状态跟踪（已在顶部声明，避免重复）

    /* 格式化函数 */
    // 按要求：电池电压为当前显示的10倍，保留一位小数；电容为当前显示的100倍
    function fmtBattery(b){ if(b===undefined) return "-"; return (b/10).toFixed(1)+"V"; }
    function fmtCapacitance(c){ if(c===undefined) return "-"; return (c/10).toFixed(1)+"V"; }
    // 通用三位小数格式化
    function fmt3(v){ if(v===undefined) return "-"; return Number(v).toFixed(3); }
    // z轴加速度：与重力加速度接近则置0（单位假定为 m/s²）
    function fmtAccZ(v){ if(v===undefined) return "-"; var g=9.80665; var th=0.5; var val=Number(v); if(Math.abs(val - g) < th) val = 0; return val.toFixed(3); }
    // 电量百分比：兼容多种字段名（battery_percent / batteryRatio / soc / battery_soc / battery_perc）
    function fmtBatteryPercent(info){
        if(!info) return "-";
        var keys = ["battery_percent", "batteryRatio", "soc", "battery_soc", "battery_perc"];
        for (var i=0;i<keys.length;i++){
            var k = keys[i];
            if (info[k]!==undefined && info[k]!==null) {
                var v = Number(info[k]);
                if (!isNaN(v)) {
                    // 若值在 0-1 之间，视为比例，转换为百分比
                    if (v<=1.0) v = v * 100.0;
                    // 夹紧到 0-100
                    if (v<0) v=0; if (v>100) v=100;
                    return Math.round(v) + "%";
                }
            }
        }
        return "-";
    }

    Rectangle { anchors.fill: parent; color: "#444" }

    Column {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8
        Row {
            width: parent.width
            spacing: 8
            Text { text: "在线车辆"; color: "#eee"; font.pixelSize: 16; verticalAlignment: Text.AlignVCenter }
            Button {
                id: startButton
                text: root.started ? "Stop" : "Start"
                onClicked: {
                    root.started = !root.started
                    // 不自动订阅：必须 Start 后再点车辆
                    root.subscribedForCurrent = false
                    root.subscribeAttempts = 0
                    subscribePoll.stop()
                    subscribeKick.stop()
                    if (typeof robotStatusModel !== 'undefined') {
                        robotStatusModel.detailEnabled = root.started
                    }
                    console.log("[QML] Start toggled", "started=", root.started)
                }
            }
        }
        ListView {
            id: list
            model: typeof robotStatusModel !== 'undefined' ? robotStatusModel : []
            width: parent.width
            height: parent.height * 0.48
            clip: true
            interactive: true
            highlightFollowsCurrentItem: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                width: list.width
                height: 40
                color: online ? (team===1?"#355":"#553") : "#4a4a4a"
                border.color: (index===list.currentIndex) ? "#ffcc66" : "transparent"
                border.width: (index===list.currentIndex) ? 2 : 0
                Row {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8
                    Text {
                        // 低电量（<40%）时变红
                        color: {
                            var info = (typeof robotStatusModel!=='undefined') ? robotStatusModel.getDetails(team, robotId) : ({});
                            var percStr = root.fmtBatteryPercent(info);
                            var percVal = -1;
                            if (percStr && percStr.endsWith("%")) { percVal = Number(percStr.slice(0, -1)); }
                            return (percVal>=0 && percVal<40) ? "#ff4d4f" : "white";
                        }
                        elide: Text.ElideRight
                        text: {
                            var info = (typeof robotStatusModel!=='undefined') ? robotStatusModel.getDetails(team, robotId) : ({});
                            var colorTxt = (team===1?"蓝":"黄");
                            var perc = root.fmtBatteryPercent(info);
                            var ip = (info && info.ip) ? info.ip : (commandClient ? (commandClient.ipPrefix + "." + robotId) : "");
                            var iplast = "-";
                            if (ip) { var parts = (""+ip).split('.'); if (parts.length>=4) iplast = parts[3]; }
                            var infrared = (info && info.infrared!==undefined) ? info.infrared : "-";
                            var imu = (info && info.have_imu!==undefined) ? (info.have_imu?"true":"false") : "-";
                            // 目标格式：“蓝5： 195:  40%；-10000 ；true”
                            return colorTxt + robotId + "： " + iplast + ":  " + perc + "；" + infrared + " ；" + imu;
                        }
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        list.currentIndex = index;
                        root.selectedTeam = team;
                        root.selectedRobot = robotId;
                        root.carSelected(robotId);
                        console.log("[QML] select", team, robotId, "started=", root.started);

                        root.subscribedForCurrent = false;
                        root.subscribeAttempts = 0;
                        if (root.started) {
                            subscribePoll.restart();
                        } else {
                            subscribePoll.stop();
                        }
                    }
                }
            }
        }
        Rectangle {
            id: detailCard
            width: parent.width
            height: parent.height - list.height - 40
            color: "#2f2f2f"; radius: 6; border.color: "#555"
            Flickable {
                anchors.fill: parent
                clip: true
                ScrollBar.vertical: ScrollBar {}
                contentWidth: col.width
                contentHeight: col.height
                Column {
                    id: col
                    anchors.margins: 8
                    spacing: 6
                    width: detailCard.width - 16
                    property var info: (root.selectedTeam && root.selectedRobot>=0 && typeof robotStatusModel!=='undefined') ? robotStatusModel.getDetails(root.selectedTeam, root.selectedRobot) : ({})
                    // 监听模型变更，主动刷新 info，确保实时更新
                    Connections {
                        target: (typeof robotStatusModel!=='undefined') ? robotStatusModel : null
                        function onDataChanged(topLeft, bottomRight, roles) {
                            if (!(root.selectedTeam && root.selectedRobot>=0)) return;
                            col.info = robotStatusModel.getDetails(root.selectedTeam, root.selectedRobot);
                        }
                    }
                    Text { text: (root.selectedTeam? (root.selectedTeam===1?"蓝":"黄") : "-") + " 队  #" + (root.selectedRobot>=0?root.selectedRobot:""); color: "#eee"; font.pixelSize: 14 }
                    // 当选中变化且已解析到 ip 时，向机器人发送一次订阅以便其向手机回报详细状态
                    Timer { id: subscribeKick; interval: 150; repeat: false; onTriggered: {
                            if (!root.started) return;
                            if (root.selectedRobot>=0) {
                                var ipCand = (col.info && col.info.ip) ? col.info.ip : (commandClient ? (commandClient.ipPrefix + "." + root.selectedRobot) : "");
                                var did = false;
                                if (ipCand) {
                                    console.log("[QML] one-shot subscribeKick", ipCand, root.selectedRobot)
                                    try { commandClient.subscribeWithIp(ipCand, root.selectedRobot); did = true; } catch(e) { /* ignore */ }
                                }
                                // 回退：同时尝试手工 IP
                                if (root.manualSubscribeIps && root.manualSubscribeIps.length>0) {
                                    for (var i=0;i<root.manualSubscribeIps.length;i++){
                                        var mip = root.manualSubscribeIps[i]; if(!mip) continue;
                                        console.log("[QML] subscribeKick manual", mip, root.selectedRobot)
                                        try { commandClient.subscribeWithIp(mip, root.selectedRobot); did = true; } catch(e) { /* ignore */ }
                                    }
                                }
                                if (did) root.subscribedForCurrent = true;
                            }
                        }
                    }
                    onInfoChanged: {
                        console.log("[QML] info changed", JSON.stringify(col.info))
                        if (!root.started) return;
                        var ipCand2 = (col.info && col.info.ip) ? col.info.ip : (commandClient ? (commandClient.ipPrefix + "." + root.selectedRobot) : "");
                        if (ipCand2) { console.log("[QML] subscribeKick restart for", ipCand2, root.selectedRobot); subscribeKick.restart() }
                    }
                    // subscribePoll 已移至根节点，便于 delegate 访问
                    Repeater {
                        model: [
                            {k:"battery", label:"电池", fmt:(v)=>root.fmtBattery(v)},
                            {k:"battery_percent", label:"电量百分比", fmt:(v)=>{ /* 此项仅当有标准字段时直接显示，否则使用综合函数 */ return (v!==undefined? (function(p){ var x=Number(p); if(isNaN(x)) return "-"; if(x<=1.0) x*=100.0; if(x<0) x=0; if(x>100) x=100; return Math.round(x)+"%"; })(v) : root.fmtBatteryPercent(col.info)); }},
                            {k:"capacitance", label:"电容", fmt:(v)=>root.fmtCapacitance(v)},
                            {k:"infrared", label:"红外时间"},
                            {k:"ip", label:"ip"},
                            {k:"Vr", label:"自转角速度Vr(rad/s)", fmt:(v)=>root.fmt3(v)},
                            // 图片中的六项：x/y/z轴角度、x/y角速度、z轴加速度
                            {k:"imu_data8", label:"x轴角度", fmt:(v)=>root.fmt3(v)},
                            {k:"imu_data9", label:"y轴角度", fmt:(v)=>root.fmt3(v)},
                            {k:"imu_data10", label:"z轴角度", fmt:(v)=>root.fmt3(v)},
                            {k:"imu_data4", label:"x轴角速度", fmt:(v)=>root.fmt3(v)},
                            {k:"imu_data5", label:"y轴角速度", fmt:(v)=>root.fmt3(v)},
                            {k:"imu_data2", label:"z轴加速度", fmt:(v)=>root.fmtAccZ(v)},
                            // 保留四个轮速度
                            {k:"wheel_ref0", label:"0号轮速度", fmt:(v)=>root.fmt3(v)},
                            {k:"wheel_ref1", label:"1号轮速度", fmt:(v)=>root.fmt3(v)},
                            {k:"wheel_ref2", label:"2号轮速度", fmt:(v)=>root.fmt3(v)},
                            {k:"wheel_ref3", label:"3号轮速度", fmt:(v)=>root.fmt3(v)},
                            // IMU状态：false为不正常，true为正常（来自 have_imu 布尔）
                            {k:"have_imu", label:"IMU状态", fmt:(v)=> (v? "true" : "false") }
                        ]
                        delegate: Rectangle {
                            width: col.width; height: 30; radius: 4
                            color: "#5fa8c6"; border.color: "#97c7dd"
                            Text { anchors.centerIn: parent; color: "#001c26"; elide: Text.ElideRight;
                                text: modelData.label + "：" + (col.info[modelData.k]!==undefined ? (modelData.fmt? modelData.fmt(col.info[modelData.k]) : col.info[modelData.k]) : "-") }
                        }
                    }
                }
            }
        }
    }

    // 定期重申 master（防止偶发丢失详细数据）：每 2s 发送一次最小订阅
    Timer {
        id: assertMaster
        interval: 2000
        repeat: true
        running: root.started
        onTriggered: {
            if (!root.subscribedForCurrent) return
            if (!(root.selectedTeam && root.selectedRobot>=0) || typeof robotStatusModel==='undefined' || !commandClient) return
            var info = robotStatusModel.getDetails(root.selectedTeam, root.selectedRobot)
            var ipCand = (info && info.ip) ? info.ip : (commandClient.ipPrefix + "." + root.selectedRobot)
            if (ipCand) {
                try { commandClient.subscribeWithIp(ipCand, root.selectedRobot) } catch(e) {}
            }
        }
    }
}
