#ifndef XML_NAVIGATOR_H
#define XML_NAVIGATOR_H

#include "parser/ast.h"
#include <pugixml.hpp>
#include <string>
#include <vector>

namespace expocli {

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

    // Evaluate WHERE expression (condition or logical combination)
    static bool evaluateWhereExpr(
        const pugi::xml_node& node,
        const WhereExpr* expr,
        size_t parentDepth = 0
    );

    // Evaluate WHERE condition on a specific node
    static bool evaluateCondition(
        const pugi::xml_node& node,
        const WhereCondition& condition
    );

    // Evaluate WHERE condition on a node with parent depth offset
    // parentDepth: number of path components already traversed to reach this node
    static bool evaluateCondition(
        const pugi::xml_node& node,
        const WhereCondition& condition,
        size_t parentDepth
    );

    // Helper to navigate nested paths (absolute from current node)
    static void findNodes(
        const pugi::xml_node& node,
        const std::vector<std::string>& path,
        size_t depth,
        std::vector<pugi::xml_node>& results
    );

    // Find nodes by partial path (suffix matching)
    // Searches entire tree for nodes where the path ending matches the given components
    // Example: ["department", "name"] matches ".../departments/department/name"
    static void findNodesByPartialPath(
        const pugi::xml_node& node,
        const std::vector<std::string>& path,
        std::vector<pugi::xml_node>& results
    );

    // Find first element with given name in XML tree (depth-first search)
    static pugi::xml_node findFirstElementByName(
        const pugi::xml_node& node,
        const std::string& name
    );

private:

    // Get value from node for comparison
    static std::string getNodeValue(
        const pugi::xml_node& node,
        const FieldPath& field
    );

    // Get value from node using relative path (skipping first 'offset' components)
    static std::string getNodeValueRelative(
        const pugi::xml_node& node,
        const FieldPath& field,
        size_t offset
    );

    // Compare values
    static bool compareValues(
        const std::string& nodeValue,
        const std::string& targetValue,
        ComparisonOp op,
        bool isNumeric
    );
};

} // namespace expocli

#endif // XML_NAVIGATOR_H
