#include "generator/xsd_schema.h"

namespace expocli {

void XsdSchema::setRootElement(std::shared_ptr<XsdElement> root) {
    root_element_ = root;
}

std::shared_ptr<XsdElement> XsdSchema::getRootElement() const {
    return root_element_;
}

void XsdSchema::setTargetNamespace(const std::string& ns) {
    target_namespace_ = ns;
}

std::string XsdSchema::getTargetNamespace() const {
    return target_namespace_;
}

} // namespace expocli
