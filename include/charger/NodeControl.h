// NodeControl.h - single sysfs node accessor.
//
// The accessor holds the node path in a std::string and performs I/O with
// C++ streams: readInt()/readStr() construct an std::ifstream over the path
// and extract with operator>>, write() constructs an std::ofstream.
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

    // A failed extraction is treated as "no value"; the previous result is
    // kept.
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
