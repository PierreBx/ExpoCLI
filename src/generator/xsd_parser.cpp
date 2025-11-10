#include "generator/xsd_parser.h"
#include <iostream>
#include <algorithm>
#include <cstring>

namespace expocli {

std::unique_ptr<XsdSchema> XsdParser::parse(const std::string& xsd_file_path) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(xsd_file_path.c_str());

    if (!result) {
        throw std::runtime_error("Failed to parse XSD file: " + std::string(result.description()));
    }

    auto schema = std::make_unique<XsdSchema>();

    // Find the schema element (handle different namespace prefixes)
    pugi::xml_node schemaNode = doc.child("xs:schema");
    if (!schemaNode) {
        schemaNode = doc.child("xsd:schema");
    }
    if (!schemaNode) {
        schemaNode = doc.child("schema");
    }

    if (!schemaNode) {
        throw std::runtime_error("No schema element found in XSD file");
    }

    // Get target namespace if present
    const char* targetNs = schemaNode.attribute("targetNamespace").value();
    if (targetNs) {
        schema->setTargetNamespace(targetNs);
    }

    // Find the root element (first xs:element)
    pugi::xml_node rootElementNode = schemaNode.child("xs:element");
    if (!rootElementNode) {
        rootElementNode = schemaNode.child("xsd:element");
    }
    if (!rootElementNode) {
        rootElementNode = schemaNode.child("element");
    }

    if (!rootElementNode) {
        throw std::runtime_error("No root element found in XSD schema");
    }

    auto rootElement = parseElement(rootElementNode, doc);
    schema->setRootElement(rootElement);

    return schema;
}

std::shared_ptr<XsdElement> XsdParser::parseElement(
    const pugi::xml_node& node,
    const pugi::xml_document& doc
) {
    auto element = std::make_shared<XsdElement>();

    // Get element name
    const char* name = node.attribute("name").value();
    if (name) {
        element->name = name;
    }

    // Get type
    const char* typeName = node.attribute("type").value();
    if (typeName) {
        element->type = parseType(typeName);
    }

    // Get minOccurs and maxOccurs
    element->minOccurs = parseOccurs(node, "minOccurs", 1);
    element->maxOccurs = parseOccurs(node, "maxOccurs", 1);

    // Check for inline complex type
    pugi::xml_node complexTypeNode = node.child("xs:complexType");
    if (!complexTypeNode) {
        complexTypeNode = node.child("xsd:complexType");
    }
    if (!complexTypeNode) {
        complexTypeNode = node.child("complexType");
    }

    if (complexTypeNode) {
        element->type = XsdType::COMPLEX;
        auto complexElement = parseComplexType(complexTypeNode, doc);
        element->children = complexElement->children;
    }

    return element;
}

std::shared_ptr<XsdElement> XsdParser::parseComplexType(
    const pugi::xml_node& complexTypeNode,
    const pugi::xml_document& doc
) {
    auto element = std::make_shared<XsdElement>();
    element->type = XsdType::COMPLEX;

    // Look for sequence
    pugi::xml_node sequenceNode = complexTypeNode.child("xs:sequence");
    if (!sequenceNode) {
        sequenceNode = complexTypeNode.child("xsd:sequence");
    }
    if (!sequenceNode) {
        sequenceNode = complexTypeNode.child("sequence");
    }

    if (sequenceNode) {
        auto sequenceElement = parseSequence(sequenceNode, doc);
        element->children = sequenceElement->children;
    }

    return element;
}

std::shared_ptr<XsdElement> XsdParser::parseSequence(
    const pugi::xml_node& sequenceNode,
    const pugi::xml_document& doc
) {
    auto element = std::make_shared<XsdElement>();

    // Parse all child elements in the sequence
    for (pugi::xml_node childNode : sequenceNode.children()) {
        std::string nodeName = childNode.name();

        if (nodeName == "xs:element" || nodeName == "xsd:element" || nodeName == "element") {
            auto childElement = parseElement(childNode, doc);
            element->children.push_back(childElement);
        }
    }

    return element;
}

XsdType XsdParser::parseType(const std::string& typeName) {
    // Remove namespace prefix if present
    std::string cleanType = typeName;
    size_t colonPos = cleanType.find(':');
    if (colonPos != std::string::npos) {
        cleanType = cleanType.substr(colonPos + 1);
    }

    // Map XSD types to our internal types
    if (cleanType == "string") return XsdType::STRING;
    if (cleanType == "int" || cleanType == "integer" || cleanType == "long" || cleanType == "short") {
        return XsdType::INTEGER;
    }
    if (cleanType == "decimal" || cleanType == "float" || cleanType == "double") {
        return XsdType::DECIMAL;
    }
    if (cleanType == "boolean") return XsdType::BOOLEAN;
    if (cleanType == "date") return XsdType::DATE;
    if (cleanType == "dateTime") return XsdType::DATETIME;

    // Default to string for unknown types
    return XsdType::STRING;
}

int XsdParser::parseOccurs(const pugi::xml_node& node, const char* attrName, int defaultValue) {
    const char* value = node.attribute(attrName).value();

    if (!value || strlen(value) == 0) {
        return defaultValue;
    }

    std::string strValue = value;

    if (strValue == "unbounded") {
        return -1;  // -1 represents unbounded
    }

    try {
        return std::stoi(strValue);
    } catch (...) {
        return defaultValue;
    }
}

} // namespace expocli
