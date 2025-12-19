#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <memory>
#include <QVariant>

class UdpSender : public QObject {
    Q_OBJECT
public:
    explicit UdpSender(QObject* parent=nullptr) : QObject(parent) {}

    void send(const QByteArray& data, const QHostAddress& addr, quint16 port){
        if(!socket_) socket_ = std::make_unique<QUdpSocket>();
        // Ensure socket ready for broadcast; bind if not bound to improve Android delivery
        if (!socket_->isOpen()) {
            // Best-effort bind AnyIPv4 ephemeral port with ShareAddress
            socket_->bind(QHostAddress::AnyIPv4, 0, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
        }
        socket_->setSocketOption(QAbstractSocket::MulticastTtlOption, QVariant(1));
        // Enable broadcast if destination is broadcast
        bool isBroadcast = addr == QHostAddress::Broadcast || addr.toString().endsWith(".255");
        if (isBroadcast) socket_->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, QVariant(64*1024));
        qint64 sent = socket_->writeDatagram(data, addr, port);
        if (sent < 0) {
            qWarning() << "udp send error" << socket_->errorString() << "to" << addr.toString() << port << "len" << data.size();
        } else {
            qInfo() << "udp sent" << sent << "bytes to" << addr.toString() << port;
        }
    }
private:
    std::unique_ptr<QUdpSocket> socket_;
};
