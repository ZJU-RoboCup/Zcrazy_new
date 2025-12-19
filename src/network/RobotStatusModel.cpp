#include "RobotStatusModel.h"
#include <QDateTime>
#include <QtMath>
#include <QDebug>

static int makeKey(int team, int robotId){ return team * 100 + robotId; }
static int orderKey(int team, int robotId){ return (team==2 ? 0 : 1) * 1000 + robotId; }
static int findInsertRow(const QList<RobotStatusEntry>& items, int team, int robotId){
    const int ok = orderKey(team, robotId);
    for (int i=0;i<items.size();++i){
        const auto &e = items.at(i);
        if (ok < orderKey(e.team, e.robotId)) return i;
    }
    return items.size();
}

// 依据总电压估算电量百分比（优先判定为4S，否则3S），采用典型锂电池OCV近似曲线做分段线性插值。
// 输入为原始电池值（10倍电压），返回0-100的整数百分比。
static int percentFromRawBattery(int raw10x){
    if (raw10x <= 0) return 0;
    const double volts = raw10x / 10.0; // 总电压
    // 简单推断电芯数：>=13.0V 视为4S，否则3S（避免将12.6V满电的3S误判为4S）
    int cells = (volts >= 13.0 ? 4 : 3);
    double perCell = volts / cells;
    // 调整后的每芯 OCV→SOC 映射（降序），确保 4S 总压 15.1V（≈3.775V/芯）≈ 40%
    // 该表更贴近你当前电池：在 3.78V/芯 设定为 40%，并在 3.80V/芯 约 45%
    static const double vTab[]    = {4.20, 4.15, 4.10, 4.05, 4.00, 3.95, 3.92, 3.88, 3.84, 3.80, 3.78, 3.75, 3.72, 3.70, 3.68, 3.65, 3.62, 3.55, 3.30};
    static const int    pTab[]    = { 100,   95,   90,   85,   80,   75,   70,   60,   50,   45,   40,   35,   30,   25,   20,   15,   10,    5,    0 };
    const int N = sizeof(vTab)/sizeof(vTab[0]);
    if (perCell >= vTab[0]) return 100;
    if (perCell <= vTab[N-1]) return 0;
    // 在表中查找区间并线性插值
    for (int i=0; i<N-1; ++i){
        double vHigh = vTab[i];
        double vLow  = vTab[i+1];
        if (perCell <= vHigh && perCell >= vLow){
            int    pHigh = pTab[i];
            int    pLow  = pTab[i+1];
            double t = (perCell - vLow) / (vHigh - vLow);
            int p = (int)qRound(pLow + t * (pHigh - pLow));
            if (p<0) p=0; if (p>100) p=100; return p;
        }
    }
    // 兜底（不应到达）
    int p = (int)qBound(0, (int)qRound((perCell - 3.30) / (4.20 - 3.30) * 100.0), 100);
    return p;
}

RobotStatusModel::RobotStatusModel(QObject* parent) : QAbstractListModel(parent) {
    m_gcTimer.setInterval(1000);
    connect(&m_gcTimer, &QTimer::timeout, this, &RobotStatusModel::pruneOffline);
    m_gcTimer.start();
    // 节流 dataChanged：将频繁更新合并为每 100ms 一次批量通知，提升多车场景下的滚动流畅度
    m_emitTimer.setInterval(100);
    m_emitTimer.setSingleShot(true);
    connect(&m_emitTimer, &QTimer::timeout, this, [this]{
        if (m_pendingEmitRows.isEmpty()) return;
        // 合并为整段通知，避免逐行触发造成 UI 卡顿
        int minRow = m_pendingEmitRows.first();
        int maxRow = m_pendingEmitRows.last();
        emit dataChanged(index(minRow), index(maxRow));
        m_pendingEmitRows.clear();
    });
}

void RobotStatusModel::setDetailEnabled(bool enabled)
{
    if (m_detailEnabled == enabled) return;
    m_detailEnabled = enabled;
    emit detailEnabledChanged();
}

int RobotStatusModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant RobotStatusModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) return {};
    const auto &e = m_items.at(index.row());
    switch(role){
    case RobotIdRole: return e.robotId;
    case TeamRole: return e.team;
    case BatteryRole: return e.battery;
    case OnlineRole: return (QDateTime::currentMSecsSinceEpoch() - e.lastUpdateMs) < 2000;
    }
    return {};
}

QHash<int, QByteArray> RobotStatusModel::roleNames() const {
    return {{RobotIdRole, "robotId"}, {TeamRole, "team"}, {BatteryRole, "battery"}, {OnlineRole, "online"}};
}

void RobotStatusModel::clear(){
    beginResetModel();
    m_items.clear();
    m_indexByKey.clear();
    endResetModel();
}

void RobotStatusModel::upsert(int team, int robotId, int battery){
    const int key = makeKey(team, robotId);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_indexByKey.contains(key)){
        int row = m_indexByKey.value(key);
        auto &e = m_items[row];
        e.battery = battery;
        // 同步写入 detail，便于 QML 统一读取，并计算百分比
        e.detail.insert("battery", battery);
        e.detail.insert("battery_percent", percentFromRawBattery(battery));
        e.lastUpdateMs = now;
        // 节流触发：记录行号并启动/延长批量定时器
        if (m_pendingEmitRows.isEmpty() || row < m_pendingEmitRows.first()) m_pendingEmitRows.prepend(row);
        if (m_pendingEmitRows.isEmpty() || row > m_pendingEmitRows.last()) m_pendingEmitRows.append(row);
        if (!m_emitTimer.isActive()) m_emitTimer.start();
    } else {
        int row = findInsertRow(m_items, team, robotId);
        beginInsertRows(QModelIndex(), row, row);
        RobotStatusEntry e; e.team = team; e.robotId = robotId; e.battery = battery; e.lastUpdateMs = now;
        e.detail.insert("battery", battery);
        e.detail.insert("battery_percent", percentFromRawBattery(battery));
        m_items.insert(row, e);
        endInsertRows();
        // 更新索引表（插入点之后的行号顺延）
        QHash<int,int> newIndex;
        for (auto it = m_indexByKey.constBegin(); it != m_indexByKey.constEnd(); ++it) {
            int oldRow = it.value();
            newIndex.insert(it.key(), oldRow + (oldRow >= row ? 1 : 0));
        }
        m_indexByKey = newIndex;
        m_indexByKey.insert(key, row);
        // 新增行后也纳入批量通知
        m_pendingEmitRows.append(row);
        if (!m_emitTimer.isActive()) m_emitTimer.start();
    }
}

void RobotStatusModel::upsertDetail(int team, int robotId, const QVariantMap &detail){
    // Stop 状态下不处理来自单播(uni)的详情，避免“抢主控”；但组播(mc/rsmc)的详情仍允许显示（如红外、battery等）。
    const QString rxSource = detail.value("rx_source").toString();
    const bool isUnicast = (rxSource == QLatin1String("uni"));
    if (!m_detailEnabled && isUnicast) return;
    const int key = makeKey(team, robotId);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    int row = -1;
    if (m_indexByKey.contains(key)) {
        row = m_indexByKey.value(key);
    } else {
        row = findInsertRow(m_items, team, robotId);
        beginInsertRows(QModelIndex(), row, row);
        RobotStatusEntry e; e.team = team; e.robotId = robotId; e.lastUpdateMs = now; m_items.append(e);
        m_items.insert(row, m_items.takeLast()); // 将新元素放到排序位置（复用e赋值）
        endInsertRows();
        // 更新索引表
        QHash<int,int> newIndex;
        for (auto it = m_indexByKey.constBegin(); it != m_indexByKey.constEnd(); ++it) {
            int oldRow = it.value();
            newIndex.insert(it.key(), oldRow + (oldRow >= row ? 1 : 0));
        }
        m_indexByKey = newIndex;
        m_indexByKey.insert(key, row);
    }
    auto &e = m_items[row];
    // keep previous snapshot for derived values
    QVariantMap prev = e.detail;
    qint64 prevTs = e.lastUpdateMs;
    e.lastUpdateMs = now;
    // merge detail keys
    for (auto it = detail.constBegin(); it != detail.constEnd(); ++it) {
        // 合并策略：仅在新值存在且合理时覆盖，避免“缺失/0”抖动
        const QString k = it.key();
        const QVariant v = it.value();
        if (k == QLatin1String("battery")) {
            int newBat = v.toInt();
            int oldBat = e.detail.value("battery", 0).toInt();
            // 若新值为0且旧值在近期为非0，则保留旧值（防止在组播/未填充时闪为0）
            if (newBat == 0 && oldBat > 0 && prevTs > 0 && (now - prevTs) < 3000) {
                // keep old
            } else {
                e.detail.insert(k, v);
                e.battery = newBat;
                e.detail.insert("battery_percent", percentFromRawBattery(newBat));
            }
        } else if (k.startsWith("imu_data") || k == QLatin1String("have_imu") || k == QLatin1String("imu_theta_z") || k == QLatin1String("imu_omega_z")) {
            // IMU相关：仅在新值提供时更新；缺失不清空，避免偶发“无IMU”覆盖
            e.detail.insert(k, v);
        } else {
            e.detail.insert(k, v);
        }
    }
    // fallback: imu_theta_z <- real_pose2 when absent
    if (!e.detail.contains("imu_theta_z") && e.detail.contains("real_pose2")) {
        e.detail.insert("imu_theta_z", e.detail.value("real_pose2"));
    }
    // fallback: imu_theta_z <- imu_data10 (desktop uses imu_data[10] for z angle)
    if (!e.detail.contains("imu_theta_z") && e.detail.contains("imu_data10")) {
        e.detail.insert("imu_theta_z", e.detail.value("imu_data10"));
    }
    // fallback: imu_omega_z <- imu_data6 when missing
    if (!e.detail.contains("imu_omega_z") && e.detail.contains("imu_data6")) {
        e.detail.insert("imu_omega_z", e.detail.value("imu_data6"));
    }
    // Build current encoder snapshot if present
    QVariantMap curEnc;
    for (int i = 0; i < 4; ++i) {
        QString key = QString("wheel_encoder%1").arg(i);
        if (detail.contains(key)) curEnc.insert(key, detail.value(key));
    }
    // derive wheel_ref* from wheel_encoder* using stable previous encoder snapshot
    // 按用户要求：四个轮速度直接与编码器数据保持一致
    if (!curEnc.isEmpty()) {
        for (int i = 0; i < 4; ++i) {
            QString key = QString("wheel_encoder%1").arg(i);
            if (curEnc.contains(key)) {
                e.detail.insert(QString("wheel_ref%1").arg(i), curEnc.value(key));
            }
        }
    }
    // 更新编码器快照（保留原有机制）
    if (!curEnc.isEmpty()) {
        e.lastEncoders = curEnc;
        e.lastEncodersTs = now;
    }
    // 标记本次包是否推导出了各来源的角速度（仅本次计算才作为候选来源）
    bool poseOmegaUpdated = false;
    bool imuAngleUpdated = false;
    bool imuOmegaUpdated = false;

    // derive angular velocity from pose heading if available in THIS packet (rad/s)
    if (detail.contains("real_pose2") && prev.contains("real_pose2") && prevTs > 0) {
        double a2 = detail.value("real_pose2").toDouble();
        double a1 = prev.value("real_pose2").toDouble();
        double d = a2 - a1;
        // unwrap to [-pi, pi]
        while (d > M_PI) d -= 2*M_PI;
        while (d < -M_PI) d += 2*M_PI;
        double dtPose = (now - prevTs) / 1000.0;
        if (dtPose > 0.0001) {
            double omega_pose = d / dtPose;
            e.detail.insert("omega_from_pose", omega_pose);
            qInfo() << "Vr pose-diff" << "a1" << a1 << "a2" << a2 << "d" << d << "dt" << dtPose << "omega" << omega_pose;
            poseOmegaUpdated = true;
        }
    }
    // fallback: derive angular velocity from IMU z-angle (imu_data10) in THIS packet (assume degrees for imu_data10)
    if (!e.detail.contains("omega_from_pose") && detail.contains("imu_data10") && prev.contains("imu_data10") && prevTs > 0) {
        double a2d = detail.value("imu_data10").toDouble();
        double a1d = prev.value("imu_data10").toDouble();
        double dd = a2d - a1d;
        while (dd > 180.0) dd -= 360.0;
        while (dd < -180.0) dd += 360.0;
        double dt = (now - prevTs) / 1000.0;
        if (dt > 0.0001) {
            double omega_from_imu_angle = (dd * M_PI / 180.0) / dt;
            e.detail.insert("omega_from_imu_angle", omega_from_imu_angle);
            qInfo() << "Vr imu-angle-diff" << "a1d" << a1d << "a2d" << a2d << "dd" << dd << "dt" << dt << "omega" << omega_from_imu_angle;
            imuAngleUpdated = true;
        }
    }
    // 额外回退：若提供了整型的 imu_theta_z（通常为角度或角度*scale），按本次包的角度差分估算角速度
    if (!e.detail.contains("omega_from_pose") && !e.detail.contains("omega_from_imu_angle")
        && detail.contains("imu_theta_z") && prev.contains("imu_theta_z") && prevTs > 0) {
        double a2d_like = detail.value("imu_theta_z").toDouble();
        double a1d_like = prev.value("imu_theta_z").toDouble();
        double dd_like = a2d_like - a1d_like;
        // 若原始值为度，使用 360 展开；若为某比例，展开仍能抑制跳变（近似处理）
        while (dd_like > 180.0) dd_like -= 360.0;
        while (dd_like < -180.0) dd_like += 360.0;
        double dt2 = (now - prevTs) / 1000.0;
        if (dt2 > 0.0001) {
            double omega_from_theta_z = (dd_like * M_PI / 180.0) / dt2;
            e.detail.insert("omega_from_imu_angle", omega_from_theta_z);
            qInfo() << "Vr imu-theta_z-diff" << "a1" << a1d_like << "a2" << a2d_like << "dd" << dd_like << "dt" << dt2 << "omega" << omega_from_theta_z;
            imuAngleUpdated = true;
        }
    }
    // derive imu omega in rad/s if imu_data6 provided (likely deg/s)
    // 优先来源1：imu_data6（多数协议中为 Z 轴角速度，单位°/s）——仅当本次包提供
    if (detail.contains("imu_data6")) {
        double degs = detail.value("imu_data6").toDouble();
        double rad = degs * M_PI / 180.0;
        e.detail.insert("imu_omega_z_rad", rad);
        qInfo() << "Vr imu_data6->rad" << "degs" << degs << "rad" << rad;
        imuOmegaUpdated = true;
    }
    // 优先来源2：imu_omega_z（若为原始整型，通常单位为°/s，统一转换到 rad/s）——仅当本次包提供
    if (detail.contains("imu_omega_z")) {
        double raw = detail.value("imu_omega_z").toDouble();
        // 简单启发：绝对值通常不超过几百，按°/s 转换；若已是 rad/s 也不影响显示，仅是再次乘以 PI/180 造成偏小，这不理想，但更常见的是°/s。
        // 为避免误判，若存在 imu_omega_z_rad，则保持更可信的 rad 值，不重复覆盖。
        if (!e.detail.contains("imu_omega_z_rad")) {
            double rad2 = raw * M_PI / 180.0;
            e.detail.insert("imu_omega_z_rad", rad2);
            qInfo() << "Vr imu_omega_z->rad" << "raw" << raw << "rad" << rad2;
            imuOmegaUpdated = true;
        }
    }
    // 选择用于显示的 Z 轴角速度：优先 真实位姿差分 -> IMU角度差分 -> IMU原生（统一 rad/s）
    bool updatedOmega = false;
    if (poseOmegaUpdated) {
        e.detail.insert("z_omega", e.detail.value("omega_from_pose"));
        e.detail.insert("Vr_source", QStringLiteral("pose"));
        qInfo() << "Vr source pose" << e.detail.value("z_omega").toDouble();
        updatedOmega = true;
    } else if (imuAngleUpdated) {
        e.detail.insert("z_omega", e.detail.value("omega_from_imu_angle"));
        e.detail.insert("Vr_source", QStringLiteral("imu_angle"));
        qInfo() << "Vr source imu_angle" << e.detail.value("z_omega").toDouble();
        updatedOmega = true;
    } else if (imuOmegaUpdated) {
        // 本次包提供了原生角速度，更新之
        double zo = e.detail.value("imu_omega_z_rad", 0.0).toDouble();
        e.detail.insert("z_omega", zo);
        e.detail.insert("Vr_source", QStringLiteral("imu_omega"));
        qInfo() << "Vr source imu_omega" << zo;
        updatedOmega = true;
    }
    // 输出 Vr（rad/s）供 QML 显示：仅在本次有新来源时更新，否则保留上一帧，避免因缺失字段被重置为 0
    if (updatedOmega) {
        double vr = e.detail.value("z_omega").toDouble();
        e.detail.insert("Vr", vr);
        qInfo() << "Vr final" << vr;
    }
    // 节流触发数据变更：统一批量通知
    if (m_pendingEmitRows.isEmpty() || row < m_pendingEmitRows.first()) m_pendingEmitRows.prepend(row);
    if (m_pendingEmitRows.isEmpty() || row > m_pendingEmitRows.last()) m_pendingEmitRows.append(row);
    if (!m_emitTimer.isActive()) m_emitTimer.start();
}

QVariantMap RobotStatusModel::getDetails(int team, int robotId) const{
    const int key = makeKey(team, robotId);
    if (!m_indexByKey.contains(key)) return {};
    const auto &e = m_items[m_indexByKey.value(key)];
    QVariantMap out = e.detail;
    out.insert("team", e.team);
    out.insert("robot_id", e.robotId);
    out.insert("battery", e.battery);
    out.insert("online", (QDateTime::currentMSecsSinceEpoch() - e.lastUpdateMs) < 2000);
    return out;
}

void RobotStatusModel::pruneOffline(){
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    bool changed = false;
    for (int i = 0; i < m_items.size(); ++i){
        if (now - m_items[i].lastUpdateMs >= 2000){
            // trigger dataChanged for online role change
            emit dataChanged(index(i), index(i));
            changed = true;
        }
    }
    if(changed){ /* already emitted per row */ }
}
