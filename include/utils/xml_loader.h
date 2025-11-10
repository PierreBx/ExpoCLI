#ifndef XML_LOADER_H
#define XML_LOADER_H

#include <pugixml.hpp>
#include <string>
#include <memory>

namespace expocli {

class XmlLoader {
public:
    // Load an XML file and return the document
    static std::unique_ptr<pugi::xml_document> load(const std::string& filepath);

    // Check if a file is a valid XML file
    static bool isXmlFile(const std::string& filepath);
};

} // namespace expocli

#endif // XML_LOADER_H
