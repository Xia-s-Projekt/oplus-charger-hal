#include "charger/NodeControl.h"

#include <fstream>

namespace vendor {
namespace oplus {
namespace hardware {
namespace charger {

NodeControl::NodeControl(const char* path) : mPath(path ? path : "") {}

NodeControl::NodeControl(const std::string& path) : mPath(path) {}

bool NodeControl::readInt(int* out) const {
    std::ifstream in(mPath.c_str());
    if (!in.is_open()) {
        return false;
    }
    int value = 0;
    in >> value;
    if (in.fail()) {
        return false;
    }
    *out = value;
    return true;
}

bool NodeControl::readStr(std::string* out) const {
    std::ifstream in(mPath.c_str());
    if (!in.is_open()) {
        return false;
    }
    std::string value;
    in >> value;
    if (in.fail()) {
        return false;
    }
    *out = value;
    return true;
}

bool NodeControl::write(const char* value) const {
    std::ofstream out(mPath.c_str());
    if (!out.is_open()) {
        return false;
    }
    out << value;
    out.flush();
    return out.good();
}

}  // namespace charger
}  // namespace hardware
}  // namespace oplus
}  // namespace vendor
