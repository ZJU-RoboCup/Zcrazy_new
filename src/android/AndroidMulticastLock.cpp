#include "AndroidMulticastLock.h"

#include <QJniObject>
#include <QtCore/QCoreApplication>
#ifdef Q_OS_ANDROID
#include <jni.h>
#endif
#include <QDebug>

AndroidMulticastLock::AndroidMulticastLock() {
#ifdef Q_OS_ANDROID
    QJniObject activity = QJniObject::callStaticObjectMethod("org/qtproject/qt/android/QtNative", "activity", "()Landroid/app/Activity;");
    if(!activity.isValid()) { qWarning() << "AndroidMulticastLock: activity invalid"; return; }

    QJniObject appContext = activity.callObjectMethod("getApplicationContext", "()Landroid/content/Context;");
    if(!appContext.isValid()) { qWarning() << "AndroidMulticastLock: context invalid"; return; }

    QJniObject wifiService = appContext.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", QJniObject::fromString("wifi").object<jstring>());
    if(!wifiService.isValid()) { qWarning() << "AndroidMulticastLock: wifiService invalid"; return; }

    // android.net.wifi.WifiManager cast
    QJniObject wifiManager(wifiService.object());
    if(!wifiManager.isValid()) { qWarning() << "AndroidMulticastLock: wifiManager invalid"; return; }

    // create multicast lock
    m_lock = wifiManager.callObjectMethod("createMulticastLock", "(Ljava/lang/String;)Landroid/net/wifi/WifiManager$MulticastLock;", QJniObject::fromString("zcrazy_mc_lock").object<jstring>());
    if(!m_lock.isValid()) { qWarning() << "AndroidMulticastLock: createMulticastLock failed"; return; }

    // Make lock non-refcounted and hold persistent reference
    m_lock.callMethod<void>("setReferenceCounted", "(Z)V", jboolean(false));
    m_lock.callMethod<void>("acquire");
    if(m_lock.isValid()) {
        m_acquired = true;
        qInfo() << "AndroidMulticastLock acquired";
    }
#else
    // Non-Android: nothing
#endif
}

AndroidMulticastLock::~AndroidMulticastLock(){
#ifdef Q_OS_ANDROID
    if(m_acquired && m_lock.isValid()) {
        m_lock.callMethod<void>("release");
        qInfo() << "AndroidMulticastLock released";
    }
#endif
}
