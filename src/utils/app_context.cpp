#include "utils/app_context.h"

namespace expocli {

void AppContext::setXsdPath(const std::string& path) {
    xsd_path_ = path;
}

std::optional<std::string> AppContext::getXsdPath() const {
    return xsd_path_;
}

bool AppContext::hasXsdPath() const {
    return xsd_path_.has_value();
}

void AppContext::setDestPath(const std::string& path) {
    dest_path_ = path;
}

std::optional<std::string> AppContext::getDestPath() const {
    return dest_path_;
}

bool AppContext::hasDestPath() const {
    return dest_path_.has_value();
}

void AppContext::setVerbose(bool verbose) {
    verbose_ = verbose;
}

bool AppContext::isVerbose() const {
    return verbose_;
}

} // namespace expocli
