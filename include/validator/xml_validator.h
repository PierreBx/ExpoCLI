#ifndef XML_VALIDATOR_H
#define XML_VALIDATOR_H

#include <string>
#include <vector>
#include <memory>
#include <pugixml.hpp>
#include "generator/xsd_schema.h"

namespace expocli {

struct ValidationError {
    std::string message;
    std::string path;       // XPath-like location in the document
    int line = -1;          // Line number if available
};

struct ValidationResult {
    bool isValid = true;
    std::vector<ValidationError> errors;
    std::vector<std::string> warnings;

    void addError(const std::string& message, const std::string& path = "") {
        isValid = false;
        errors.push_back({message, path, -1});
    }

    void addWarning(const std::string& message) {
        warnings.push_back(message);
    }
};

class XmlValidator {
public:
    XmlValidator() = default;

    // Validate an XML file against an XSD schema
    ValidationResult validateFile(
        const std::string& xmlFile,
        const std::string& xsdFile
    );

    // Validate multiple files
    std::vector<std::pair<std::string, ValidationResult>> validateFiles(
        const std::vector<std::string>& xmlFiles,
        const std::string& xsdFile
    );

    // Expand glob patterns to file list
    static std::vector<std::string> expandPattern(const std::string& pattern);

private:
    ValidationResult validateAgainstSchema(
        const pugi::xml_document& doc,
        const XsdSchema& schema
    );

    bool validateElement(
        const pugi::xml_node& node,
        const std::shared_ptr<XsdElement>& schemaElement,
        ValidationResult& result,
        const std::string& path
    );

    bool validateAttributes(
        const pugi::xml_node& node,
        const std::shared_ptr<XsdElement>& schemaElement,
        ValidationResult& result,
        const std::string& path
    );

    bool validateChildren(
        const pugi::xml_node& node,
        const std::shared_ptr<XsdElement>& schemaElement,
        ValidationResult& result,
        const std::string& path
    );

    bool matchesType(const std::string& value, XsdType type);
};

} // namespace expocli

#endif // XML_VALIDATOR_H
