#ifndef XML_NAVIGATOR_H
#define XML_NAVIGATOR_H

#include "parser/ast.h"
#include <pugixml.hpp>
#include <string>
#include <vector>

namespace xmlquery {

// Represents a single result from XML traversal
struct XmlResult {
    std::string filename;
    std::string value;
};

class XmlNavigator {
public:
    // Navigate XML document and extract values matching the field path
    static std::vector<XmlResult> extractValues(
        const pugi::xml_document& doc,
        const std::string& filename,
        const FieldPath& field
    );

    // Evaluate WHERE condition on a specific node
    static bool evaluateCondition(
        const pugi::xml_node& node,
        const WhereCondition& condition
    );

    // Helper to navigate nested paths
    static void findNodes(
        const pugi::xml_node& node,
        const std::vector<std::string>& path,
        size_t depth,
        std::vector<pugi::xml_node>& results
    );

private:

    // Get value from node for comparison
    static std::string getNodeValue(
        const pugi::xml_node& node,
        const FieldPath& field
    );

    // Compare values
    static bool compareValues(
        const std::string& nodeValue,
        const std::string& targetValue,
        ComparisonOp op,
        bool isNumeric
    );
};

} // namespace xmlquery

#endif // XML_NAVIGATOR_H
