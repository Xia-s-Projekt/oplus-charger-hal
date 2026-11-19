// ChgDetectionCheck.h - uevent driven charge detection.
//
// RegisterEvent() adds a listener; a netlink socket feeds a single epoll loop
// that dispatches power_supply uevents to the registered listeners. Each
// listener updates the HAL state it owns.
//
// The fast-charge listener is FastChargeDetector::refresh().
#pragma once

#include <semaphore.h>
#include <sys/epoll.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {

class ChgDetectionCheck;

enum class EventWakeup {
    LEVEL = 0,  // EPOLLIN
    EDGE = 1,   // EPOLLIN | EPOLLET
};

using EventHandler = std::function<void(ChgDetectionCheck*, unsigned int)>;

class ChgDetectionCheck {
public:
    ChgDetectionCheck();
    ~ChgDetectionCheck();

    // Register a listener for a file descriptor.  The listener is invoked from
    // MainLoop with the owner and the epoll event data.
    bool registerEvent(int fd, EventHandler handler, EventWakeup wakeup);

    // The uevent socket reader: receives one netlink message and hands the
    // power_supply ones to the registered listeners.
    void ueventEvent(unsigned int fd);

    // epoll setup; returns 0 on success.
    int ueventInit();

    // epoll_wait loop; returns when epoll_wait fails hard.
    void mainLoop();

    int epollFd() const { return mEpollFd; }
    const std::string& lastUevent() const { return mLastUevent; }

    // Post the power_supply semaphore on a power_supply uevent.
    void notifyPowerSupply();

    // Wait for the power_supply semaphore.
    void waitPowerSupply();

    // Open a netlink uevent socket. This uses an in-tree helper instead of
    // libhardware's:
    //   socket(AF_NETLINK, SOCK_DGRAM|SOCK_CLOEXEC, NETLINK_KOBJECT_UEVENT)
    //   SO_RCVBUF (with SO_RCVBUFFORCE when the kernel doubled it)
    //   SO_PASSCRED when credentials are wanted
    //   bind(nl_pid = 0, nl_groups = ~0)  <- all groups
    static int oplusUeventOpenSocket(int bufferSize, bool passCredentials);

    // Parse "KEY=VALUE\n..." uevent text into a map.
    static std::vector<std::pair<std::string, std::string>> parseUevent(
            const std::string& raw);

private:
    struct EventEntry {
        ChgDetectionCheck* owner = nullptr;
        int fd = -1;
        EventHandler handler;
    };

    void dispatch(const struct epoll_event& event);

    int mEpollFd = -1;
    int mUeventFd = -1;
    sem_t mPowerSupplySem;
    bool mSemInited = false;
    std::string mLastUevent;
    std::vector<EventEntry> mHandlers;
    std::mutex mLock;
};

}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
