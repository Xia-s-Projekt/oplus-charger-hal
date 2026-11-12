// NodeControl.h - single sysfs node accessor.
//
// Reconstructed from the shipped binary
// erofs/vendor/bin/hw/vendor.oplus.hardware.charger-V3-service:
//
//   NodeControl::NodeControl(char const*)   ELF 0x405b0
//   NodeControl::readStr()                 ELF 0x40c60
//   NodeControl::readInt()                 ELF 0x41224
//   NodeControl::write(char const*)        ELF 0x41688
//
// The shipped class holds the node path in a std::string member and does the
// I/O with C++ streams: readInt()/readStr() construct an std::ifstream over
// the path and extract with operator>>, write() constructs an std::ofstream.
// Evidence: the ctor stores the path into the std::string at this+0
// (stp xzr,xzr,[x0] / str xzr,[x0,#0x10] then the libc++ SSO copy), and all
// three accessors call std::ios_base::init before touching the stream.
#pragma once

#include <string>

namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {

class NodeControl {
public:
    explicit NodeControl(const char* path);
    explicit NodeControl(const std::string& path);

    // Extraction failures give the stream's fail state; the shipped code
    // treats a failed extraction as "no value" and keeps the previous result.
    bool readInt(int* out) const;
    bool readStr(std::string* out) const;
    bool write(const char* value) const;

    const std::string& path() const { return mPath; }

private:
    std::string mPath;
};

}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
