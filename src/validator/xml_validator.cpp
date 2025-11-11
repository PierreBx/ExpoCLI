#include "validator/xml_validator.h"
#include "generator/xsd_parser.h"
#include <pugixml.hpp>
#include <filesystem>
#include <glob.h>
#include <iostream>
#include <regex>
#include <algorithm>
#include <set>
#include <cstring>

namespace expocli {

std::vector<std::string> XmlValidator::expandPattern(const std::string& pattern) {
    std::vector<std::string> files;

    // Check if pattern is a directory
    if (std::filesystem::is_directory(pattern)) {
        // List all XML files in the directory
        for (const auto& entry : std::filesystem::directory_iterator(pattern)) {
            if (entry.is_regular_file() && entry.path().extension() == ".xml") {
                files.push_back(entry.path().string());
            }
        }
        std::sort(files.begin(), files.end());
        return files;
    }

    // Check if pattern is a single file
    if (std::filesystem::exists(pattern) && std::filesystem::is_regular_file(pattern)) {
        files.push_back(pattern);
        return files;
    }

    // Use glob for pattern matching
    glob_t globResult;
    memset(&globResult, 0, sizeof(globResult));

    int returnValue = glob(pattern.c_str(), GLOB_TILDE, nullptr, &globResult);

    if (returnValue == 0) {
        for (size_t i = 0; i < globResult.gl_pathc; ++i) {
            std::string path = globResult.gl_pathv[i];
            if (std::filesystem::is_regular_file(path)) {
                files.push_back(path);
            }
        }
    }

    globfree(&globResult);
    std::sort(files.begin(), files.end());
    return files;
}

ValidationResult XmlValidator::validateFile(
    const std::string& xmlFile,
    const std::string& xsdFile
) {
    ValidationResult result;

    // Check if XML file exists
    if (!std::filesystem::exists(xmlFile)) {
        result.addError("XML file does not exist: " + xmlFile);
        return result;
    }

    // Load XML file
    pugi::xml_document doc;
    pugi::xml_parse_result parseResult = doc.load_file(xmlFile.c_str());

    if (!parseResult) {
        result.addError(
            std::string("Failed to parse XML file: ") + parseResult.description(),
            xmlFile
        );
        return result;
    }

    // Parse XSD schema
    std::unique_ptr<XsdSchema> schema;
    try {
        schema = XsdParser::parse(xsdFile);
    } catch (const std::exception& e) {
        result.addError(std::string("Failed to parse XSD schema: ") + e.what());
        return result;
    }

    // Validate XML against schema
    return validateAgainstSchema(doc, *schema);
}

std::vector<std::pair<std::string, ValidationResult>> XmlValidator::validateFiles(
    const std::vector<std::string>& xmlFiles,
    const std::string& xsdFile
) {
    std::vector<std::pair<std::string, ValidationResult>> results;

    for (const auto& xmlFile : xmlFiles) {
        ValidationResult result = validateFile(xmlFile, xsdFile);
        results.push_back({xmlFile, result});
    }

    return results;
}

ValidationResult XmlValidator::validateAgainstSchema(
    const pugi::xml_document& doc,
    const XsdSchema& schema
) {
    ValidationResult result;

    auto rootElement = schema.getRootElement();
    if (!rootElement) {
        result.addError("Schema has no root element defined");
        return result;
    }

    // Find the root node in the XML document
    pugi::xml_node xmlRoot = doc.first_child();

    // Skip XML declaration if present
    while (xmlRoot && xmlRoot.type() != pugi::node_element) {
        xmlRoot = xmlRoot.next_sibling();
    }

    if (!xmlRoot) {
        result.addError("XML document has no root element");
        return result;
    }

    // Check root element name matches
    if (xmlRoot.name() != rootElement->name) {
        result.addError(
            "Root element name mismatch. Expected: " + rootElement->name +
            ", Found: " + std::string(xmlRoot.name()),
            "/" + std::string(xmlRoot.name())
        );
        return result;
    }

    // Validate the root element recursively
    validateElement(xmlRoot, rootElement, result, "/" + rootElement->name);

    return result;
}

bool XmlValidator::validateElement(
    const pugi::xml_node& node,
    const std::shared_ptr<XsdElement>& schemaElement,
    ValidationResult& result,
    const std::string& path
) {
    bool valid = true;

    // Validate attributes
    if (!validateAttributes(node, schemaElement, result, path)) {
        valid = false;
    }

    // Validate content based on type
    if (schemaElement->type == XsdType::COMPLEX) {
        // Validate child elements
        if (!validateChildren(node, schemaElement, result, path)) {
            valid = false;
        }
    } else {
        // Simple type - validate text content
        std::string textValue = node.text().as_string();

        // Empty text is allowed if element has minOccurs=0
        if (textValue.empty() && !schemaElement->isOptional()) {
            result.addError(
                "Required element is empty",
                path
            );
            valid = false;
        } else if (!textValue.empty() && !matchesType(textValue, schemaElement->type)) {
            result.addError(
                "Value does not match expected type: " + textValue,
                path
            );
            valid = false;
        }
    }

    return valid;
}

bool XmlValidator::validateAttributes(
    const pugi::xml_node& node,
    const std::shared_ptr<XsdElement>& schemaElement,
    ValidationResult& result,
    const std::string& path
) {
    bool valid = true;

    // Check for required attributes
    for (const auto& schemaAttr : schemaElement->attributes) {
        pugi::xml_attribute xmlAttr = node.attribute(schemaAttr->name.c_str());

        if (!xmlAttr) {
            if (!schemaAttr->isOptional()) {
                result.addError(
                    "Missing required attribute: " + schemaAttr->name,
                    path
                );
                valid = false;
            }
        } else {
            // Validate attribute value type
            std::string attrValue = xmlAttr.value();
            if (!matchesType(attrValue, schemaAttr->type)) {
                result.addError(
                    "Attribute '" + schemaAttr->name +
                    "' has invalid value type: " + attrValue,
                    path
                );
                valid = false;
            }
        }
    }

    // Check for unexpected attributes (not in schema)
    for (pugi::xml_attribute attr : node.attributes()) {
        std::string attrName = attr.name();
        bool found = false;
        for (const auto& schemaAttr : schemaElement->attributes) {
            if (schemaAttr->name == attrName) {
                found = true;
                break;
            }
        }
        if (!found) {
            result.addWarning(
                "Unexpected attribute '" + attrName + "' at " + path
            );
        }
    }

    return valid;
}

bool XmlValidator::validateChildren(
    const pugi::xml_node& node,
    const std::shared_ptr<XsdElement>& schemaElement,
    ValidationResult& result,
    const std::string& path
) {
    bool valid = true;

    // Count occurrences of each child element
    std::map<std::string, int> childCounts;
    for (pugi::xml_node child : node.children()) {
        if (child.type() == pugi::node_element) {
            childCounts[child.name()]++;
        }
    }

    // Validate each schema child element
    for (const auto& schemaChild : schemaElement->children) {
        int count = childCounts[schemaChild->name];

        // Check minOccurs
        if (count < schemaChild->minOccurs) {
            result.addError(
                "Element '" + schemaChild->name + "' appears " +
                std::to_string(count) + " times, but minOccurs is " +
                std::to_string(schemaChild->minOccurs),
                path
            );
            valid = false;
        }

        // Check maxOccurs (if not unbounded)
        if (schemaChild->maxOccurs != -1 && count > schemaChild->maxOccurs) {
            result.addError(
                "Element '" + schemaChild->name + "' appears " +
                std::to_string(count) + " times, but maxOccurs is " +
                std::to_string(schemaChild->maxOccurs),
                path
            );
            valid = false;
        }

        // Mark as validated
        childCounts.erase(schemaChild->name);
    }

    // Check for unexpected child elements
    for (const auto& [childName, count] : childCounts) {
        result.addWarning(
            "Unexpected element '" + childName + "' (appears " +
            std::to_string(count) + " times) at " + path
        );
    }

    // Recursively validate each child element
    for (pugi::xml_node child : node.children()) {
        if (child.type() != pugi::node_element) {
            continue;
        }

        std::string childName = child.name();
        std::string childPath = path + "/" + childName;

        // Find matching schema element
        std::shared_ptr<XsdElement> matchingSchema;
        for (const auto& schemaChild : schemaElement->children) {
            if (schemaChild->name == childName) {
                matchingSchema = schemaChild;
                break;
            }
        }

        if (matchingSchema) {
            if (!validateElement(child, matchingSchema, result, childPath)) {
                valid = false;
            }
        }
        // else: already reported as unexpected element
    }

    return valid;
}

bool XmlValidator::matchesType(const std::string& value, XsdType type) {
    if (value.empty()) {
        return true; // Empty values are checked separately
    }

    switch (type) {
        case XsdType::STRING:
            return true; // Any string is valid

        case XsdType::INTEGER: {
            try {
                size_t pos;
                std::stoll(value, &pos);
                return pos == value.length();
            } catch (...) {
                return false;
            }
        }

        case XsdType::DECIMAL: {
            try {
                size_t pos;
                std::stod(value, &pos);
                return pos == value.length();
            } catch (...) {
                return false;
            }
        }

        case XsdType::BOOLEAN: {
            return value == "true" || value == "false" ||
                   value == "1" || value == "0";
        }

        case XsdType::DATE: {
            // Basic ISO date format: YYYY-MM-DD
            std::regex datePattern(R"(\d{4}-\d{2}-\d{2})");
            return std::regex_match(value, datePattern);
        }

        case XsdType::DATETIME: {
            // Basic ISO datetime format: YYYY-MM-DDTHH:MM:SS
            std::regex datetimePattern(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2})");
            return std::regex_match(value, datetimePattern);
        }

        case XsdType::COMPLEX:
            return true; // Complex types are validated structurally

        default:
            return true;
    }
}

} // namespace expocli
