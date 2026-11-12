#include "charger/ChgDetectionCheck.h"

#include <errno.h>
#include <linux/netlink.h>
#include <semaphore.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <vector>

namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {

namespace {
const char kPowerSupplySubsystem[] = "SUBSYSTEM=power_supply";
const int kUeventBufferSize = 2048;  // the shipped UeventEvent reads 0x800
}  // namespace

ChgDetectionCheck::ChgDetectionCheck() {
    mEpollFd = epoll_create1(EPOLL_CLOEXEC);
    if (mSemInited != true) {
        if (sem_init(&mPowerSupplySem, 0, 0) == 0) {
            mSemInited = true;
        }
    }
}

ChgDetectionCheck::~ChgDetectionCheck() {
    if (mUeventFd >= 0) {
        close(mUeventFd);
        mUeventFd = -1;
    }
    if (mEpollFd >= 0) {
        close(mEpollFd);
        mEpollFd = -1;
    }
    if (mSemInited) {
        sem_destroy(&mPowerSupplySem);
        mSemInited = false;
    }
}

int ChgDetectionCheck::oplusUeventOpenSocket(int bufferSize, bool passCredentials) {
    int fd = socket(AF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC, NETLINK_KOBJECT_UEVENT);
    if (fd < 0) {
        return -1;
    }

    int size = bufferSize;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) < 0) {
        close(fd);
        return -1;
    }

    // The kernel doubles the requested buffer; if it is still too small ask
    // for the exact size with SO_RCVBUFFORCE, exactly as the shipped
    // oplus_uevent_open_socket does.
    socklen_t len = sizeof(size);
    int actual = 0;
    if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &actual, &len) < 0) {
        close(fd);
        return -1;
    }
    if (size * 2 <= actual) {
        if (setsockopt(fd, SOL_SOCKET, SO_RCVBUFFORCE, &size, sizeof(size)) < 0) {
            close(fd);
            return -1;
        }
    }

    int creds = passCredentials ? 1 : 0;
    setsockopt(fd, SOL_SOCKET, SO_PASSCRED, &creds, sizeof(creds));

    struct sockaddr_nl addr;
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = 0;
    addr.nl_groups = 0xffffffff;  // every group, as the shipped code binds
    if (bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int ChgDetectionCheck::ueventInit() {
    if (mEpollFd < 0) {
        return -1;
    }
    mUeventFd = oplusUeventOpenSocket(kUeventBufferSize, false);
    if (mUeventFd < 0) {
        return -1;
    }
    return registerEvent(mUeventFd,
                         [](ChgDetectionCheck* owner, unsigned int fd) {
                             owner->ueventEvent(fd);
                         },
                         EventWakeup::LEVEL)
                   ? 0
                   : -1;
}

bool ChgDetectionCheck::registerEvent(int fd, EventHandler handler, EventWakeup wakeup) {
    if (fd < 0 || mEpollFd < 0) {
        return false;
    }
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = (wakeup == EventWakeup::EDGE) ? (EPOLLIN | EPOLLET) : EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        return false;
    }
    std::lock_guard<std::mutex> guard(mLock);
    EventEntry entry;
    entry.owner = this;
    entry.fd = fd;
    entry.handler = std::move(handler);
    mHandlers.push_back(std::move(entry));
    return true;
}

void ChgDetectionCheck::ueventEvent(unsigned int fd) {
    char buffer[kUeventBufferSize];
    ssize_t n = recv((int)fd, buffer, sizeof(buffer) - 1, 0);
    if (n < 1) {
        return;
    }
    buffer[n] = '\0';

    std::string raw(buffer, (size_t)n);
    {
        std::lock_guard<std::mutex> guard(mLock);
        mLastUevent = raw;
    }

    // The shipped UeventEvent walks the NUL separated uevent records looking
    // for "SUBSYSTEM=power_supply" and posts the semaphore when it finds one.
    if (raw.find(kPowerSupplySubsystem) != std::string::npos) {
        notifyPowerSupply();
    }
}

void ChgDetectionCheck::notifyPowerSupply() {
    if (mSemInited) {
        sem_post(&mPowerSupplySem);
    }
}

void ChgDetectionCheck::waitPowerSupply() {
    if (mSemInited) {
        sem_wait(&mPowerSupplySem);
    }
}

void ChgDetectionCheck::dispatch(const struct epoll_event& event) {
    std::vector<EventEntry> snapshot;
    {
        std::lock_guard<std::mutex> guard(mLock);
        snapshot = mHandlers;
    }
    for (auto& entry : snapshot) {
        if (entry.fd == event.data.fd && entry.handler) {
            entry.handler(entry.owner, event.data.u32);
        }
    }
}

void ChgDetectionCheck::mainLoop() {
    struct epoll_event events[16];
    for (;;) {
        int n = epoll_wait(mEpollFd, events, 16, -1);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        for (int i = 0; i < n; i++) {
            dispatch(events[i]);
        }
    }
}

std::vector<std::pair<std::string, std::string>> ChgDetectionCheck::parseUevent(
        const std::string& raw) {
    std::vector<std::pair<std::string, std::string>> out;
    size_t pos = 0;
    while (pos < raw.size()) {
        size_t end = raw.find('\n', pos);
        if (end == std::string::npos) {
            end = raw.size();
        }
        std::string line = raw.substr(pos, end - pos);
        pos = end + 1;
        if (line.empty()) {
            continue;
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos) {
            out.emplace_back(line, std::string());
        } else {
            out.emplace_back(line.substr(0, eq), line.substr(eq + 1));
        }
    }
    return out;
}

}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
