#pragma once

#include <QJniObject>

// Lightweight RAII holder for Android WifiManager MulticastLock
class AndroidMulticastLock {
public:
    AndroidMulticastLock();
    ~AndroidMulticastLock();

    bool acquired() const { return m_acquired; }

private:
    bool m_acquired = false;
    // Hold a persistent reference to prevent GC releasing the lock
    QJniObject m_lock;
};
