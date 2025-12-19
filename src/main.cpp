#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QNetworkInterface>

#include "CommandClient.h"
#include "network/RobotStatusModel.h"
#include "network/StatusReceiver.h"
#if defined(Q_OS_ANDROID)
#include "android/AndroidMulticastLock.h"
#include <android/log.h>
#endif

// Simple Qt message handler to log runtime issues when running as a GUI app
static void fileMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
#if defined(Q_OS_ANDROID)
    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    const QString baseDir = QCoreApplication::applicationDirPath();
#endif
    static QFile logFile(baseDir + "/zcrazy_run.log");
    if (!logFile.isOpen()) {
        // Ensure directory exists (Android: AppDataLocation may need mkdir)
        QDir().mkpath(QFileInfo(logFile).absolutePath());
        logFile.open(QIODevice::Append | QIODevice::Text);
    }
    QTextStream ts(&logFile);
    const char *typeStr = "INFO";
    switch (type) {
    case QtDebugMsg: typeStr = "DEBUG"; break;
    case QtInfoMsg: typeStr = "INFO"; break;
    case QtWarningMsg: typeStr = "WARN"; break;
    case QtCriticalMsg: typeStr = "CRIT"; break;
    case QtFatalMsg: typeStr = "FATAL"; break;
    }
    ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " [" << typeStr << "] "
       << msg;
    if (context.file)
        ts << " (" << context.file << ":" << context.line << ")";
    ts << "\n";
    ts.flush();
#if defined(Q_OS_ANDROID)
    // Also print to Android logcat for real-time debugging
    int prio = ANDROID_LOG_INFO;
    switch (type) {
    case QtDebugMsg: prio = ANDROID_LOG_DEBUG; break;
    case QtInfoMsg: prio = ANDROID_LOG_INFO; break;
    case QtWarningMsg: prio = ANDROID_LOG_WARN; break;
    case QtCriticalMsg: prio = ANDROID_LOG_ERROR; break;
    case QtFatalMsg: prio = ANDROID_LOG_FATAL; break;
    }
    __android_log_print(prio, "zcrazy", "%s", msg.toUtf8().constData());
#endif
    if (type == QtFatalMsg) abort();
}

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    // Install log handler early
    qInstallMessageHandler(fileMessageHandler);
    QGuiApplication app(argc, argv);
    QGuiApplication::setQuitOnLastWindowClosed(false);

    QQmlApplicationEngine engine;
    qInfo() << "App started; setting up QML engine";

    // Expose command client to QML
    CommandClient commandClient;
    engine.rootContext()->setContextProperty("commandClient", &commandClient);

    // Detect local IPv4 and set ipPrefix accordingly for subnet broadcast
    QString detectedIp;
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if (!(iface.flags() & QNetworkInterface::IsUp) || (iface.flags() & QNetworkInterface::IsLoopBack))
            continue;
        for (const QNetworkAddressEntry &e : iface.addressEntries()) {
            const QHostAddress addr = e.ip();
            if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
                const QString s = addr.toString();
                if (s != QLatin1String("127.0.0.1")) { detectedIp = s; break; }
            }
        }
        if (!detectedIp.isEmpty()) break;
    }
    if (!detectedIp.isEmpty()) {
        const QStringList parts = detectedIp.split('.');
        if (parts.size() == 4) {
            const QString prefix = parts[0] + "." + parts[1] + "." + parts[2];
            qInfo() << "Local IPv4:" << detectedIp << " ipPrefix->" << prefix;
            commandClient.setIpPrefix(prefix);
        } else {
            qInfo() << "Local IPv4:" << detectedIp << " (prefix not set)";
        }
    } else {
        qWarning() << "No non-loopback IPv4 detected; keep default ipPrefix";
    }

    // Expose status model and start multicast receiver
    RobotStatusModel statusModel;
    engine.rootContext()->setContextProperty("robotStatusModel", &statusModel);
#if defined(Q_OS_ANDROID)
    // Acquire multicast lock on Android to ensure UDP multicast delivery
    static AndroidMulticastLock mcLock; // keep alive for app lifetime
    if (!mcLock.acquired()) {
        qWarning() << "MulticastLock not acquired";
    }
#endif
    StatusReceiver statusRx;
    QObject::connect(&statusRx, &StatusReceiver::multicastStatus, &statusModel, &RobotStatusModel::upsert);
    QObject::connect(&statusRx, &StatusReceiver::robotDetail, &statusModel, &RobotStatusModel::upsertDetail);
    statusRx.start();

    // Periodically re-detect local IPv4 and update ipPrefix if changed (e.g., Wi-Fi reconnect or DHCP renewal)
    QTimer netTimer; netTimer.setInterval(5000);
    QObject::connect(&netTimer, &QTimer::timeout, &app, [&commandClient, &statusRx]{
        QString detectedIp;
        for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
            if (!(iface.flags() & QNetworkInterface::IsUp) || (iface.flags() & QNetworkInterface::IsLoopBack))
                continue;
            for (const QNetworkAddressEntry &e : iface.addressEntries()) {
                const QHostAddress addr = e.ip();
                if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
                    const QString s = addr.toString();
                    if (s != QLatin1String("127.0.0.1")) { detectedIp = s; break; }
                }
            }
            if (!detectedIp.isEmpty()) break;
        }
        if (!detectedIp.isEmpty()) {
            const QStringList parts = detectedIp.split('.');
            if (parts.size() == 4) {
                const QString prefix = parts[0] + "." + parts[1] + "." + parts[2];
                static QString lastPrefix;
                if (prefix != lastPrefix) {
                    qInfo() << "IPv4 change detected:" << detectedIp << " new ipPrefix->" << prefix;
                    commandClient.setIpPrefix(prefix);
                    lastPrefix = prefix;
                    // Refresh multicast joins in case Wi-Fi iface changed
                    statusRx.refreshJoins();
                }
            }
        }
    });
    netTimer.start();

    // Load the main QML via module URI to avoid resource prefix mismatch
    // Note: Do NOT auto-exit on objectCreated(nullptr) because we attempt a fallback load below.
    qInfo() << "Loading QML module ZCrazy/Main";
    engine.loadFromModule("ZCrazy", "Main");
    if (engine.rootObjects().isEmpty()) {
        qWarning() << "Module load failed; falling back to qrc path qrc:/ZCrazy/qml/Main.qml";
        engine.load(QUrl(QStringLiteral("qrc:/ZCrazy/qml/Main.qml")));
    }
    if (engine.rootObjects().isEmpty()) {
        qWarning() << "Fallback Main.qml also failed; using diagnostic qrc:/ZCrazy/qml/MainSimple.qml";
        engine.load(QUrl(QStringLiteral("qrc:/ZCrazy/qml/MainSimple.qml")));
    }
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "All QML loads failed; exiting";
        return -1;
    }
    qInfo() << "Engine load finished; object count:" << engine.rootObjects().size();

    return app.exec();
}
