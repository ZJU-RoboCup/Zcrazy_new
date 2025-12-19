#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QVariant>
#include <QTimer>
#include <QDateTime>

// 仅解析 Multicast_Status 的少量字段（使用 protobuf tag 扫描，避免依赖 protoc）
// fields in Multicast_Status (proto):
// 1: ip (varint) -> tag 0x08
// 3: team (varint) -> tag 0x18 (1=BLUE,2=YELLOW)
// 4: robot_id (varint) -> tag 0x20
// 5: battery (varint) -> tag 0x28
class StatusReceiver : public QObject {
    Q_OBJECT
public:
    explicit StatusReceiver(QObject* parent = nullptr);

signals:
    void multicastStatus(int team, int robotId, int battery);
    void robotDetail(int team, int robotId, const QVariantMap &detail);

public slots:
    void start();
    void refreshJoins();

private slots:
    void onReadyRead();

private:
    bool parseMulticastStatus(const QByteArray& datagram, int &team, int &robotId, int &battery, QVariantMap *detailOut);
    bool parseRobotStatus(const QByteArray& datagram, int &team, int &robotId, QVariantMap &detailOut);
    bool parseRobotsStatusContainer(const QByteArray& datagram, const char* tag); // repeated Robot_Status wrapper
    static bool readVarint(const QByteArray& buf, int &i, quint64 &out);
    static qint64 zigzagDecode(quint64 v) { return (qint64)((v >> 1) ^ (~(v & 1) + 1)); }
    void processSocket(QUdpSocket &sock, const char* tag);
    QUdpSocket m_sock;        // multicast status (13134)
    QUdpSocket m_sockUni;     // unicast robot status (14134)
    QUdpSocket m_sockRsMc;    // robot status via multicast (14134, optional)
    qint64 m_lastRxMs = 0;    // last time any datagram was received
    QTimer m_watchTimer;      // watches for inactivity and re-joins multicast
};
