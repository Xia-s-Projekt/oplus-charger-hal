// ChgDetectionCheck.h - uevent driven charge detection.
//
// Reconstructed from the shipped binary
// erofs/vendor/bin/hw/vendor.oplus.hardware.charger-V3-service:
//
//   ChgDetectionCheck::RegisterEvent(int, std::function, EventWakeup)  ELF 0x2af24
//   ChgDetectionCheck::UeventEvent(unsigned int)                      ELF 0x2b140
//   ChgDetectionCheck::UeventInit()                                    ELF 0x2b21c
//   ChgDetectionCheck::MainLoop()                                      ELF 0x2b394
//   PowerSupplyUeventMonitorThread(void*)                              ELF 0x2b4c0
//   oplus_uevent_open_socket(int, bool)                                ELF 0x1a418
//
// Structure recovered from the decompiled bodies:
//   this+0x78  sem_t            semaphore posted on a power_supply uevent
//   this+0xcc  int              epoll fd
//   this+0xd0  vector begin     EventHandler*  {owner, fd, std::function}
//   this+0xd8  vector end
//   this+0xe0  vector capacity
//
// This is the Oplus counterpart of the Transsion trancharge design, but with
// a different dispatch shape:
//
//   trancharge : one uevent thread, a 59-entry {match-string, handler} table,
//                strstr() over the uevent, handler fills TranChargeInfo,
//                the registered client callback is notified.
//   oplus     : one epoll loop, a vector of std::function listeners, the
//                listener is called with the uevent, the listener updates the
//                HAL's own state.
//
// The fast-charge listener is FastChargeDetector::refresh() - the Oplus
// equivalent of trancharge's TransferPump_Express handler.
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

    // Post the power_supply semaphore (used by the shipped UeventEvent).
    void notifyPowerSupply();

    // Wait for the power_supply semaphore.
    void waitPowerSupply();

    // Open a netlink uevent socket.  The shipped binary has its own copy of
    // this (oplus_uevent_open_socket) instead of libhardware's helper:
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
