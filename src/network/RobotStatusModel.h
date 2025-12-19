#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QTimer>
#include <QVariant>

struct RobotStatusEntry {
    int robotId = -1; // 0..15
    int team = 0;     // 1=BLUE, 2=YELLOW
    int battery = 0;  // raw int from proto (10x volts)
    qint64 lastUpdateMs = 0;
    QVariantMap detail; // optional rich fields parsed from Robot_Status/Multicast_Status
    QVariantMap lastEncoders; // wheel_encoder0..3 snapshot
    qint64 lastEncodersTs = 0; // timestamp when lastEncoders updated
};

class RobotStatusModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool detailEnabled READ detailEnabled WRITE setDetailEnabled NOTIFY detailEnabledChanged)
public:
    enum Roles { RobotIdRole = Qt::UserRole + 1, TeamRole, BatteryRole, OnlineRole };

    explicit RobotStatusModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void clear();
    Q_INVOKABLE QVariantMap getDetails(int team, int robotId) const;

    // key: team*100 + robotId to avoid collision
    void upsert(int team, int robotId, int battery);
    void upsertDetail(int team, int robotId, const QVariantMap &detail);

    bool detailEnabled() const { return m_detailEnabled; }
    void setDetailEnabled(bool enabled);

signals:
    void detailEnabledChanged();

private slots:
    void pruneOffline();

private:
    QList<RobotStatusEntry> m_items;
    QHash<int, int> m_indexByKey; // key -> row
    QTimer m_gcTimer;
    // 批量 dataChanged 节流用定时器与待发行区间
    QTimer m_emitTimer;
    QList<int> m_pendingEmitRows;
    bool m_detailEnabled = false;
};
