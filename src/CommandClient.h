#pragma once
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMutex>
#include "network/UdpSender.h"

// Minimal command client: stores params and sends a placeholder payload via UDP to target IP.
// Next step: replace placeholder payload with protobuf Robot_Command.
class CommandClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString ipPrefix READ ipPrefix WRITE setIpPrefix NOTIFY ipPrefixChanged)
public:
    explicit CommandClient(QObject* parent=nullptr);

    Q_INVOKABLE void updateCommandParams(int robotID,
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
                                         bool controlAllWhichTeam);
    Q_INVOKABLE void sendCommand();
    Q_INVOKABLE void subscribeWithIp(const QString &ip, int robotId);
    Q_INVOKABLE void setNeedChangeTeam(bool v) {
        QMutexLocker lock(&mtx_);
        needChangeTeam_ = v;
    }

    QString ipPrefix() const { return ipPrefix_; } 
    void setIpPrefix(const QString& p) { if (ipPrefix_!=p){ ipPrefix_=p; emit ipPrefixChanged(); } }

signals:
    void ipPrefixChanged();

private:
    struct Params {
        int robotID = 0; // We will treat this as the last octet for now
        double vx=0, vy=0, vr=0;
        double ctrl=0; bool mode=false; bool shoot=false; double power=0;
        bool useImu=false; double angle=0; bool controlAll=false; bool controlAllWhichTeam=false;
    } params_;

    QString ipPrefix_ = "192.168.31"; // match your existing default
    UdpSender sender_;
    QMutex mtx_;
    bool needChangeTeam_ = false;
};
