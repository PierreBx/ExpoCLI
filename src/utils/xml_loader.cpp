#include "utils/xml_loader.h"
#include <algorithm>

namespace expocli {

std::unique_ptr<pugi::xml_document> XmlLoader::load(const std::string& filepath) {
    auto doc = std::make_unique<pugi::xml_document>();

    pugi::xml_parse_result result = doc->load_file(filepath.c_str());

    if (!result) {
        throw std::runtime_error("Failed to load XML file: " + filepath +
                               "\nError: " + result.description());
    }

    return doc;
}

bool XmlLoader::isXmlFile(const std::string& filepath) {
    // Check file extension
    if (filepath.length() < 4) return false;

    std::string ext = filepath.substr(filepath.length() - 4);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".xml";
}

} // namespace expocli
