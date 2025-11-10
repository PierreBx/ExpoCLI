#ifndef XSD_PARSER_H
#define XSD_PARSER_H

#include "xsd_schema.h"
#include <pugixml.hpp>
#include <string>
#include <memory>

namespace expocli {

class XsdParser {
public:
    // Parse an XSD file and return the schema model
    static std::unique_ptr<XsdSchema> parse(const std::string& xsd_file_path);

private:
    static std::shared_ptr<XsdElement> parseElement(
        const pugi::xml_node& node,
        const pugi::xml_document& doc
    );

    static std::shared_ptr<XsdElement> parseComplexType(
        const pugi::xml_node& complexTypeNode,
        const pugi::xml_document& doc
    );

    static std::shared_ptr<XsdElement> parseSequence(
        const pugi::xml_node& sequenceNode,
        const pugi::xml_document& doc
    );

    static XsdType parseType(const std::string& typeName);
    static int parseOccurs(const pugi::xml_node& node, const char* attrName, int defaultValue);
};

} // namespace expocli

#endif // XSD_PARSER_H
