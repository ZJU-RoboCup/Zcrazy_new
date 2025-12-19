#include "CommandClient.h"
#include <QHostAddress>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QtGlobal>
#include <QTimer>
#include <QPointer>

CommandClient::CommandClient(QObject* parent) : QObject(parent) {}

void CommandClient::updateCommandParams(int robotID,
                                        double velX,
                                        double velY,
                                        double velR,
                                        double ctrl,
                                        bool mode,
                                        bool shoot,
                                        double power,
                                        bool useImu,
                                        double angle,
                                        bool controlAll,
                                        bool controlAllWhichTeam)
{
    QMutexLocker lock(&mtx_);
    params_.robotID = robotID;
    params_.vx = velX; params_.vy = velY; params_.vr = velR;
    params_.ctrl = ctrl; params_.mode = mode; params_.shoot = shoot; params_.power = power;
    params_.useImu = useImu; params_.angle = angle;
    params_.controlAll = controlAll; params_.controlAllWhichTeam = controlAllWhichTeam;
}

void CommandClient::sendCommand()
{
    // Placeholder payload: JSON for smoke test. Replace with protobuf bytes later.
    Params p;
    {
        QMutexLocker lock(&mtx_);
        p = params_;
    }

    QJsonObject obj;
    obj["robot_id"] = p.robotID;
    obj["vx"] = p.vx; obj["vy"] = p.vy; obj["vr"] = p.vr;
    obj["ctrl"] = p.ctrl; obj["mode"] = p.mode; obj["shoot"] = p.shoot; obj["power"] = p.power;
    obj["use_imu"] = p.useImu; obj["angle"] = p.angle;
    obj["control_all"] = p.controlAll; obj["control_all_which_team"] = p.controlAllWhichTeam;
    {
        QMutexLocker lock(&mtx_);
        obj["need_change_team"] = needChangeTeam_;
    }

    QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    // For now we assume robotID equals the last IP octet (as in your Python code semantics with pb_info.ip)
    const QString ip = ipPrefix_ + "." + QString::number(p.robotID);
    sender_.send(payload, QHostAddress(ip), 14234);
}

static void appendVarint(QByteArray &out, quint64 v){
    while (true){
        quint8 b = v & 0x7F;
        v >>= 7;
        if (v) { out.append(char(b | 0x80)); }
        else { out.append(char(b)); break; }
    }
}

// Send a minimal Robot_Command protobuf: fields {1: robot_id, 6: need_report=true, 12: comm_type=UDP_WIFI(3)}
void CommandClient::subscribeWithIp(const QString &ip, int robotId){
    qInfo() << "subscribeWithIp start" << "target" << ip << "robotId" << robotId;
    QByteArray pb;
    // field 1 (robot_id), varint
    pb.append(char((1<<3) | 0)); appendVarint(pb, quint64(robotId));
    // field 6 (need_report) = true
    pb.append(char((6<<3) | 0)); appendVarint(pb, 1);
    // field 12 (comm_type) = 3 (UDP_WIFI)
    pb.append(char((12<<3) | 0)); appendVarint(pb, 3);
    // field 15 (isdebug) = true, optional but harmless
    pb.append(char((15<<3) | 0)); appendVarint(pb, 1);
    qInfo() << "send Robot_Command" << "len" << pb.size() << "to" << ip << 14234;
    sender_.send(pb, QHostAddress(ip), 14234);

    // Robust retries to ensure master switch on receive_port (14234)
    // Retry at t+500ms and then every 1000ms up to 15s
    {
        QPointer<UdpSender> sp = &sender_;
        const QString targetIp = ip;
        const QByteArray pbCopy = pb;
        // t + 500ms quick retry
        QTimer::singleShot(500, this, [sp, pbCopy, targetIp]() {
            if (!sp) return; qInfo() << "retry Robot_Command t+" << 500 << "ms to" << targetIp << 14234;
            sp->send(pbCopy, QHostAddress(targetIp), 14234);
        });
        // t + 1000..15000ms retries
        for (int t = 1000; t <= 15000; t += 1000) {
            QTimer::singleShot(t, this, [sp, pbCopy, targetIp, t]() {
                if (!sp) return; qInfo() << "retry Robot_Command t+" << t << "ms to" << targetIp << 14234;
                sp->send(pbCopy, QHostAddress(targetIp), 14234);
            });
        }
    }

    // Additionally, broadcast a short "master subscription" heartbeat to 12476 as observed on desktop
    // Payload derived from captured broadcast: 32 bytes repeating header + zeros
    QByteArray hb;
    // fd 01 fe fc 00 20 00 02 00 00 00 20 00 00 00 01
    const unsigned char prefix[16] = {0xfd,0x01,0xfe,0xfc,0x00,0x20,0x00,0x02,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x01};
    hb.append(reinterpret_cast<const char*>(prefix), 16);
    // pad to 32 bytes (observed Len=32)
    hb.append(QByteArray(16, '\0'));
    // Send to global broadcast and subnet broadcast for reliability
    qInfo() << "send 12476 heartbeat" << "len" << hb.size() << "to" << "255.255.255.255" << 12476;
    sender_.send(hb, QHostAddress("255.255.255.255"), 12476);
    const QString subnetBc = ipPrefix_ + ".255";
    qInfo() << "send 12476 heartbeat subnet" << "to" << subnetBc << 12476;
    sender_.send(hb, QHostAddress(subnetBc), 12476);
    // Also send unicast heartbeat directly to robot IP (helps if broadcast is blocked)
    qInfo() << "send 12476 heartbeat unicast" << "to" << ip << 12476;
    sender_.send(hb, QHostAddress(ip), 12476);
    // retry heartbeats to improve master switch success (t+500ms, t+1500ms)
    QPointer<UdpSender> sp = &sender_;
    const QString bc1 = QStringLiteral("255.255.255.255");
    const QString bc2 = subnetBc;
    QByteArray hbCopy = hb; // capture by value for lambdas
    // Extend retries: every 1000ms up to 15s, send to broadcast, subnet, and unicast
    for (int t = 1000; t <= 15000; t += 1000) {
        QTimer::singleShot(t, this, [sp, hbCopy, bc1, bc2, ip, t]() {
            if(!sp) return; qInfo() << "retry 12476 heartbeat t+" << t << "ms";
            sp->send(hbCopy, QHostAddress(bc1), 12476);
            sp->send(hbCopy, QHostAddress(bc2), 12476);
            sp->send(hbCopy, QHostAddress(ip), 12476);
        });
    }
    qInfo() << "subscribeWithIp done";
}
