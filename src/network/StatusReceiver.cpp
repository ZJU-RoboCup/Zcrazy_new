#include "StatusReceiver.h"
#include <QNetworkDatagram>
#include <QDebug>
#include <QNetworkInterface>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QtCore/QCoreApplication>
#endif

// Multicast group & port (match desktop Python)
static const QHostAddress MC_GROUP("225.225.225.225");
static const quint16 MC_PORT = 13134;
static const quint16 UNI_PORT = 14134; // unicast Robot_Status port (from desktop SINGLE_PORT)

StatusReceiver::StatusReceiver(QObject* parent) : QObject(parent) {
    connect(&m_sock, &QUdpSocket::readyRead, this, &StatusReceiver::onReadyRead);
    connect(&m_sockUni, &QUdpSocket::readyRead, this, &StatusReceiver::onReadyRead);
    connect(&m_sockRsMc, &QUdpSocket::readyRead, this, &StatusReceiver::onReadyRead);
    // Watchdog: if no packets for a while, refresh multicast joins
    m_watchTimer.setInterval(5000);
    connect(&m_watchTimer, &QTimer::timeout, this, [this]{
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (m_lastRxMs > 0 && (now - m_lastRxMs) > 10000) {
            qWarning() << "RX inactivity" << (now - m_lastRxMs) << "ms; refreshing multicast joins";
            refreshJoins();
        }
    });
}

void StatusReceiver::start(){
#ifdef Q_OS_ANDROID
    // Acquire Android WifiManager MulticastLock so the device can receive multicast UDP
    QJniObject activity = QJniObject::callStaticObjectMethod("org/qtproject/qt/android/QtNative", "activity", "()Landroid/app/Activity;");
    if (activity.isValid()) {
        QJniObject context = activity;
        QJniObject wifiService = context.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", QJniObject::fromString("wifi").object<jstring>());
        if (wifiService.isValid()) {
            QJniObject wifiManager = wifiService;
            QJniObject lock = wifiManager.callObjectMethod("createMulticastLock", "(Ljava/lang/String;)Landroid/net/wifi/WifiManager$MulticastLock;", QJniObject::fromString("zcrazy-mc").object<jstring>());
            if (lock.isValid()) {
                lock.callMethod<void>("setReferenceCounted", "(Z)V", jboolean(false));
                lock.callMethod<void>("acquire");
                qInfo() << "Android MulticastLock acquired";
            } else {
                qWarning() << "Failed to create MulticastLock";
            }
        } else {
            qWarning() << "WifiManager service unavailable";
        }
    } else {
        qWarning() << "Android activity not available for MulticastLock";
    }
#endif
    // Log local IPv4 interfaces for diagnostics
    const auto ifaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : ifaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp)) continue;
        QStringList ips;
        for (const QNetworkAddressEntry &e : iface.addressEntries()) {
            if (e.ip().protocol() == QAbstractSocket::IPv4Protocol)
                ips << e.ip().toString();
        }
        if (!ips.isEmpty()) qInfo() << "iface" << iface.humanReadableName() << "IPv4" << ips.join(',');
    }
    if (!m_sock.bind(QHostAddress::AnyIPv4, MC_PORT, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qWarning() << "Failed to bind multicast port" << MC_PORT << m_sock.errorString();
    } else {
        // Increase receive buffer to reduce risk of drops under burst
        m_sock.setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 1<<20);
        // Prefer joining on Wi-Fi interface to avoid Android dropping some groups
        QNetworkInterface wifiIf;
        for (const QNetworkInterface &iface : ifaces) {
            if (!(iface.flags() & QNetworkInterface::IsUp)) continue;
            if (iface.flags() & QNetworkInterface::IsLoopBack) continue;
            if (iface.humanReadableName().contains("wlan", Qt::CaseInsensitive) || iface.humanReadableName().contains("wifi", Qt::CaseInsensitive)) {
                wifiIf = iface; break;
            }
        }
        bool joined = false;
        if (wifiIf.isValid()) {
            joined = m_sock.joinMulticastGroup(MC_GROUP, wifiIf);
            if (!joined) qWarning() << "Join multicast on Wi-Fi iface failed" << wifiIf.humanReadableName() << m_sock.errorString();
        }
        if (!joined) {
            joined = m_sock.joinMulticastGroup(MC_GROUP);
        }
        if (!joined) {
            qWarning() << "Failed to join multicast group" << MC_GROUP << m_sock.errorString();
        } else {
            qInfo() << "Joined multicast group" << MC_GROUP.toString() << "port" << MC_PORT << "iface" << (wifiIf.isValid()?wifiIf.humanReadableName():QStringLiteral("auto"));
        }
    }
    if (!m_sockUni.bind(QHostAddress::AnyIPv4, UNI_PORT, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qWarning() << "Failed to bind unicast status port" << UNI_PORT << m_sockUni.errorString();
    } else {
        m_sockUni.setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 1<<20);
        qInfo() << "Listening unicast robot status port" << UNI_PORT;
    }

    // Optional: also listen Robot_Status via multicast on same group at port 14134
    const bool enableRsMc = qEnvironmentVariableIntValue("ZCRAZY_ENABLE_RS_MC") != 0;
    if (enableRsMc) {
        if (!m_sockRsMc.bind(QHostAddress::AnyIPv4, UNI_PORT, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
            qWarning() << "Failed to bind RS multicast port" << UNI_PORT << m_sockRsMc.errorString();
        } else {
            m_sockRsMc.setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 1<<20);
            QNetworkInterface wifiIf2;
            for (const QNetworkInterface &iface : ifaces) {
                if (!(iface.flags() & QNetworkInterface::IsUp)) continue;
                if (iface.flags() & QNetworkInterface::IsLoopBack) continue;
                if (iface.humanReadableName().contains("wlan", Qt::CaseInsensitive) || iface.humanReadableName().contains("wifi", Qt::CaseInsensitive)) {
                    wifiIf2 = iface; break;
                }
            }
            bool joined2 = false;
            if (wifiIf2.isValid()) {
                joined2 = m_sockRsMc.joinMulticastGroup(MC_GROUP, wifiIf2);
                if (!joined2) qWarning() << "Join RS multicast on Wi-Fi iface failed" << wifiIf2.humanReadableName() << m_sockRsMc.errorString();
            }
            if (!joined2) {
                joined2 = m_sockRsMc.joinMulticastGroup(MC_GROUP);
            }
            if (!joined2) {
                qWarning() << "Failed to join RS multicast group" << MC_GROUP << m_sockRsMc.errorString();
            } else {
                qInfo() << "Joined RS multicast" << MC_GROUP.toString() << "port" << UNI_PORT << "iface" << (wifiIf2.isValid()?wifiIf2.humanReadableName():QStringLiteral("auto"));
            }
        }
    } else {
        qInfo() << "RS multicast (14134) disabled by default; set ZCRAZY_ENABLE_RS_MC=1 to enable";
    }

    // Start inactivity watchdog after sockets are bound
    m_watchTimer.start();
}
void StatusReceiver::refreshJoins(){
    // Re-join multicast on current Wi-Fi iface
    const auto ifaces = QNetworkInterface::allInterfaces();
    QNetworkInterface wifiIf;
    for (const QNetworkInterface &iface : ifaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp)) continue;
        if (iface.flags() & QNetworkInterface::IsLoopBack) continue;
        if (iface.humanReadableName().contains("wlan", Qt::CaseInsensitive) || iface.humanReadableName().contains("wifi", Qt::CaseInsensitive)) {
            wifiIf = iface; break;
        }
    }
    if (m_sock.state() == QAbstractSocket::BoundState) {
        // try leave then join
        m_sock.leaveMulticastGroup(MC_GROUP);
        bool joined = false;
        if (wifiIf.isValid()) {
            joined = m_sock.joinMulticastGroup(MC_GROUP, wifiIf);
            if (!joined) qWarning() << "Re-join multicast on Wi-Fi failed" << wifiIf.humanReadableName() << m_sock.errorString();
        }
        if (!joined) {
            joined = m_sock.joinMulticastGroup(MC_GROUP);
        }
        qInfo() << "Refreshed multicast group" << MC_GROUP.toString() << "ok=" << joined;
    }
    if (m_sockRsMc.state() == QAbstractSocket::BoundState) {
        m_sockRsMc.leaveMulticastGroup(MC_GROUP);
        bool joined2 = false;
        if (wifiIf.isValid()) {
            joined2 = m_sockRsMc.joinMulticastGroup(MC_GROUP, wifiIf);
            if (!joined2) qWarning() << "Re-join RS multicast on Wi-Fi failed" << wifiIf.humanReadableName() << m_sockRsMc.errorString();
        }
        if (!joined2) joined2 = m_sockRsMc.joinMulticastGroup(MC_GROUP);
        qInfo() << "Refreshed RS multicast" << MC_GROUP.toString() << "ok=" << joined2;
    }
}

void StatusReceiver::onReadyRead(){
    processSocket(m_sock, "mc");
    processSocket(m_sockUni, "uni");
    // Process Robot_Status multicast rebroadcast (from desktop forwarder)
    processSocket(m_sockRsMc, "rsmc");
}

void StatusReceiver::processSocket(QUdpSocket &sock, const char* tag){
    while (sock.hasPendingDatagrams()) {
        QNetworkDatagram d = sock.receiveDatagram();
        m_lastRxMs = QDateTime::currentMSecsSinceEpoch();
        // 降低日志频率：仅在调试时打开详细 datagram 打印（通过环境变量控制）
        static bool verbose = qEnvironmentVariableIntValue("ZCRAZY_VERBOSE_RX") != 0;
        if (verbose) qInfo() << tag << "datagram" << d.senderAddress().toString() << d.senderPort() << "->" << d.destinationAddress().toString() << d.destinationPort() << "len" << d.data().size();
        int team=0, robotId=-1, battery=0; QVariantMap detail;
        bool msOk = parseMulticastStatus(d.data(), team, robotId, battery, &detail);
        QVariantMap rsDetail; int rsTeam=0, rsId=-1; bool rsOk = parseRobotStatus(d.data(), rsTeam, rsId, rsDetail);
        if(!msOk && !rsOk){
            // 调试：打印未解析数据报的前32字节，便于定位字段问题（如 team==0 或载荷异常）
            if (verbose) {
                QByteArray head = d.data().left(32);
                qWarning() << tag << "unparsed datagram first32=" << head.toHex(' ');
            }
            // try container wrapper Robots_Status { repeated Robot_Status robots_status = 1; }
            if(parseRobotsStatusContainer(d.data(), tag)) continue; // signals emitted inside
        }
        if (msOk && robotId>=0 && team!=0) {
            if (verbose) qInfo() << "mc rx team" << team << "id" << robotId << "bat" << battery;
            emit multicastStatus(team, robotId, battery);
            if (rsOk && rsId==robotId && rsTeam==team) {
                for (auto it = rsDetail.constBegin(); it != rsDetail.constEnd(); ++it) detail.insert(it.key(), it.value());
            }
            // 统一以数据报源地址作为 IP，避免 ip_last 异常（例如为 1）
            const QString sip = d.senderAddress().toString();
            detail.insert("ip", sip);
            detail.insert("rx_source", QString::fromLatin1(tag, int(qstrlen(tag))));
            if (!detail.isEmpty()) {
                if (verbose) { QStringList keys; for (auto it = detail.constBegin(); it != detail.constEnd(); ++it) keys << it.key(); qInfo() << "detail merged keys" << keys.join(','); }
                emit robotDetail(team, robotId, detail);
            }
        } else if (rsOk && rsId>=0 && rsTeam!=0) {
            int bat = rsDetail.value("battery").toInt();
            // 统一用数据报源地址作为 IP
            rsDetail.insert("ip", d.senderAddress().toString());
            rsDetail.insert("rx_source", QString::fromLatin1(tag, int(qstrlen(tag))));
            if (verbose) { QStringList keys; for (auto it = rsDetail.constBegin(); it != rsDetail.constEnd(); ++it) keys << it.key(); qInfo() << tag << " rs rx team" << rsTeam << "id" << rsId << "keys" << keys.join(',') << "len" << d.data().size(); }
            emit multicastStatus(rsTeam, rsId, bat);
            emit robotDetail(rsTeam, rsId, rsDetail);
        }
    }
}

// Very small varint parser (protobuf base-128)
bool StatusReceiver::readVarint(const QByteArray& buf, int &i, quint64 &out){
    out = 0; int shift = 0;
    while (i < buf.size()) {
        unsigned char c = buf[i++];
        out |= (quint64)(c & 0x7F) << shift;
        if (!(c & 0x80)) return true;
        shift += 7;
        if (shift > 63) return false;
    }
    return false;
}

bool StatusReceiver::parseMulticastStatus(const QByteArray& datagram, int &team, int &robotId, int &battery, QVariantMap *detailOut){
    team = 0; robotId = -1; battery = 0; if(detailOut) detailOut->clear();
    bool hasField = false; int ipLast = -1; quint64 v=0; quint64 len=0;
    for (int i=0; i < datagram.size();) {
        unsigned char tag = datagram[i++];
        int fieldNumber = tag >> 3; int wireType = tag & 0x07;
        switch(fieldNumber){
        case 1: // ip (int32)
            if (wireType!=0 || !readVarint(datagram,i,v)) return false; ipLast = (int)zigzagDecode(v); if(detailOut) detailOut->insert("ip_last", ipLast); break;
        case 3: // team
            if (wireType!=0 || !readVarint(datagram,i,v)) return false; team = (int)v; hasField = true; break;
        case 4: // robot_id
            if (wireType!=0 || !readVarint(datagram,i,v)) return false; robotId = (int)v; hasField = true; break;
        case 5: // battery
            if (wireType!=0 || !readVarint(datagram,i,v)) return false; battery = (int)zigzagDecode(v); hasField = true; if(detailOut) detailOut->insert("battery", battery); break;
        case 6: // capacitance
            if (wireType!=0 || !readVarint(datagram,i,v)) return false; if(detailOut) detailOut->insert("capacitance", (int)zigzagDecode(v)); break;
        case 7: // vision_valid
            if (wireType!=0 || !readVarint(datagram,i,v)) return false; if(detailOut) detailOut->insert("vision_valid", (bool)v); break;
        case 8: // infrared
            if (wireType!=0 || !readVarint(datagram,i,v)) return false; if(detailOut) detailOut->insert("infrared", (int)zigzagDecode(v)); break;
        case 9: // have_imu
            if (wireType!=0 || !readVarint(datagram,i,v)) return false; if(detailOut) detailOut->insert("have_imu", (bool)v); break;
        default:
            if (wireType == 0) { if(!readVarint(datagram,i,v)) return false; }
            else if (wireType == 2) { if(!readVarint(datagram,i,len)) return false; i += (int)len; }
            else if (wireType == 5) { i += 4; }
            else if (wireType == 1) { i += 8; }
            else { return false; }
        }
    }
    // proto3 默认值 0 不会序列化：若缺失 robot_id，按 0 处理
    if (robotId < 0) robotId = 0;
    // 不在此处设置 ip（统一由 processSocket 用数据报源地址写入）
    return hasField && team!=0 && robotId>=0;
}

// Parse Robots_Status wrapper: field 1 length-delimited repeated Robot_Status
bool StatusReceiver::parseRobotsStatusContainer(const QByteArray& datagram, const char* srcTag){
    for(int i=0; i<datagram.size();){
        unsigned char pbTag = datagram[i++]; int fieldNumber = pbTag >> 3; int wireType = pbTag & 0x07; quint64 len=0; quint64 v=0;
        if(fieldNumber==1 && wireType==2){
            if(!readVarint(datagram,i,len)) return false; int end = i + (int)len; // single Robot_Status or first of repeated packed? In proto repeated means multiple tags
            QByteArray sub = datagram.mid(i, (int)len); i = end;
            int team=0, rid=-1; QVariantMap detail; bool ok = parseRobotStatus(sub, team, rid, detail);
            if(ok){
                int bat = detail.value("battery").toInt();
                QStringList keys; for (auto it = detail.constBegin(); it != detail.constEnd(); ++it) keys << it.key();
                qInfo() << "robots_status item team" << team << "id" << rid << "keys" << keys.join(',');
                detail.insert("rx_source", QString::fromLatin1(srcTag, int(qstrlen(srcTag))));
                emit multicastStatus(team, rid, bat);
                emit robotDetail(team, rid, detail);
            }
        } else if(fieldNumber==1 && wireType==0){ // unlikely; skip varint
            if(!readVarint(datagram,i,v)) return false;
        } else if(wireType==2){ // skip other length-delimited
            if(!readVarint(datagram,i,len)) return false; i += (int)len;
        } else if(wireType==0){ if(!readVarint(datagram,i,v)) return false; }
        else if(wireType==5){ i+=4; }
        else if(wireType==1){ i+=8; }
        else { return false; }
    }
    return true; // if structure parsed
}

bool StatusReceiver::parseRobotStatus(const QByteArray& datagram, int &team, int &robotId, QVariantMap &detailOut){
    team = 0; robotId = -1; detailOut.clear(); int battery=0; bool hasFields=false;
    for (int i=0; i < datagram.size();) {
        unsigned char tag = datagram[i++]; int fieldNumber = tag >> 3; int wireType = tag & 0x07; quint64 v=0; quint64 len=0;
        switch(fieldNumber){
        case 1: if(wireType!=0 || !readVarint(datagram,i,v)) return false; robotId=(int)v; hasFields=true; break;
        case 2: if(wireType!=0 || !readVarint(datagram,i,v)) return false; detailOut.insert("infrared", (int)zigzagDecode(v)); break;
        case 3: if(wireType!=0 || !readVarint(datagram,i,v)) return false; detailOut.insert("flat_kick", (int)zigzagDecode(v)); break;
        case 4: if(wireType!=0 || !readVarint(datagram,i,v)) return false; detailOut.insert("chip_kick", (int)zigzagDecode(v)); break;
        case 5: if(wireType!=0 || !readVarint(datagram,i,v)) return false; detailOut.insert("imu_theta_z", (int)zigzagDecode(v)); break;
        case 6: if(wireType!=0 || !readVarint(datagram,i,v)) return false; battery = (int)zigzagDecode(v); detailOut.insert("battery", battery); break;
        case 7: if(wireType!=0 || !readVarint(datagram,i,v)) return false; detailOut.insert("capacitance", (int)zigzagDecode(v)); break;
        case 8: // odom (length-delimited) - skip content for now
            if (wireType!=2 || !readVarint(datagram,i,len)) return false; i += (int)len; break;
        case 9: // wheel_encoder repeated sint32 (packed)
            if (wireType == 2) { if(!readVarint(datagram,i,len)) return false; int end=i+(int)len; int idx=0; while(i<end){ if(!readVarint(datagram,i,v)) return false; detailOut.insert(QString("wheel_encoder%1").arg(idx++), (int)zigzagDecode(v)); } }
            else if(wireType==0) { int idx = detailOut.size(); if(!readVarint(datagram,i,v)) return false; detailOut.insert(QString("wheel_encoder%1").arg(idx), (int)zigzagDecode(v)); }
            else return false; break;
        case 10: if(wireType!=0 || !readVarint(datagram,i,v)) return false; team = (int)v; hasFields=true; break;
        case 13: if(wireType!=0 || !readVarint(datagram,i,v)) return false; detailOut.insert("have_imu", (bool)v); break;
        case 14: if(wireType!=0 || !readVarint(datagram,i,v)) return false; detailOut.insert("imu_omega_z", (int)zigzagDecode(v)); break;
        case 11: // imu_data repeated float (packed fixed32)
            if (wireType==2){ if(!readVarint(datagram,i,len)) return false; int end=i+(int)len; int idx=0; while(i+3<end){
                quint32 fv = (quint8)datagram[i] | ((quint8)datagram[i+1] << 8) | ((quint8)datagram[i+2] << 16) | ((quint8)datagram[i+3] << 24); i+=4;
                float f; memcpy(&f, &fv, sizeof(float)); detailOut.insert(QString("imu_data%1").arg(idx++), f);
            } }
            else if(wireType==5){ // single float
                if(i+4>datagram.size()) return false; quint32 fv = (quint8)datagram[i] | ((quint8)datagram[i+1] << 8) | ((quint8)datagram[i+2] << 16) | ((quint8)datagram[i+3] << 24); i+=4; float f; memcpy(&f, &fv, sizeof(float)); detailOut.insert("imu_data0", f);
            } else return false; break;
        case 12: // real_pose repeated float (packed fixed32)
            if (wireType==2){ if(!readVarint(datagram,i,len)) return false; int end=i+(int)len; int idx=0; while(i+3<end){
                quint32 fv = (quint8)datagram[i] | ((quint8)datagram[i+1] << 8) | ((quint8)datagram[i+2] << 16) | ((quint8)datagram[i+3] << 24); i+=4;
                float f; memcpy(&f, &fv, sizeof(float)); if(idx<3) detailOut.insert(QString("real_pose%1").arg(idx), f); idx++; }
            } else if (wireType==5) { if(i+4>datagram.size()) return false; quint32 fv = (quint8)datagram[i] | ((quint8)datagram[i+1] << 8) | ((quint8)datagram[i+2] << 16) | ((quint8)datagram[i+3] << 24); i+=4; float f; memcpy(&f, &fv, sizeof(float)); detailOut.insert("real_pose0", f); }
            else return false; break;
        case 15: // wheel_ref repeated sint32 (packed)
            if (wireType==2){ if(!readVarint(datagram,i,len)) return false; int end=i+(int)len; int idx=0; while(i<end){ if(!readVarint(datagram,i,v)) return false; if(idx<4) detailOut.insert(QString("wheel_ref%1").arg(idx), (int)zigzagDecode(v)); idx++; } }
            else if (wireType==0){ int idx = 0; if(detailOut.contains("wheel_ref0")) { while(detailOut.contains(QString("wheel_ref%1").arg(idx))) idx++; } if(!readVarint(datagram,i,v)) return false; if(idx<4) detailOut.insert(QString("wheel_ref%1").arg(idx), (int)zigzagDecode(v)); }
            else return false; break;
        default:
            if (wireType == 0) { if(!readVarint(datagram,i,v)) return false; }
            else if (wireType == 2) { if(!readVarint(datagram,i,len)) return false; i += (int)len; }
            else if (wireType == 5) { i += 4; }
            else if (wireType == 1) { i += 8; }
            else { return false; }
        }
    }
    // 若缺失 robot_id（id=0 的默认值被省略），按 0 处理
    if (robotId < 0) robotId = 0;
    return hasFields && robotId>=0 && team!=0;
}
