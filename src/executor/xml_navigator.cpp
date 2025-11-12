#include "executor/xml_navigator.h"
#include <stdexcept>
#include <typeinfo>
#include <functional>
#include <regex>

namespace expocli {

std::vector<XmlResult> XmlNavigator::extractValues(
    const pugi::xml_document& doc,
    const std::string& filename,
    const FieldPath& field
) {
    std::vector<XmlResult> results;

    // Handle FILE_NAME special case
    if (field.include_filename) {
        results.push_back({filename, filename});
        return results;
    }

    // Handle attribute extraction (@attribute)
    if (field.is_attribute) {
        // Search for all nodes in the document and extract the attribute
        std::function<void(const pugi::xml_node&)> findAllWithAttribute =
            [&](const pugi::xml_node& node) {
                if (!node) return;

                // Check if this node has the requested attribute
                pugi::xml_attribute attr = node.attribute(field.attribute_name.c_str());
                if (attr) {
                    std::string value = attr.value();
                    if (!value.empty()) {
                        results.push_back({filename, value});
                    }
                }

                // Recursively search all children
                for (pugi::xml_node child : node.children()) {
                    findAllWithAttribute(child);
                }
            };

        findAllWithAttribute(doc);
        return results;
    }

    if (field.components.empty()) {
        return results;
    }

    // Shorthand: if only one component, search for all occurrences in entire tree
    if (field.components.size() == 1) {
        std::function<void(const pugi::xml_node&)> findAllByName =
            [&](const pugi::xml_node& node) {
                if (!node) return;

                // Check if current node matches
                if (std::string(node.name()) == field.components[0]) {
                    std::string value = node.child_value();
                    if (!value.empty()) {
                        results.push_back({filename, value});
                    }
                }

                // Recursively search all children
                for (pugi::xml_node child : node.children()) {
                    findAllByName(child);
                }
            };

        findAllByName(doc);
        return results;
    }

    // Find all nodes matching the full path
    std::vector<pugi::xml_node> nodes;
    findNodes(doc, field.components, 0, nodes);

    // Extract values from found nodes
    for (const auto& node : nodes) {
        if (node) {
            std::string value = node.child_value();
            if (!value.empty()) {
                results.push_back({filename, value});
            }
        }
    }

    return results;
}

bool XmlNavigator::evaluateWhereExpr(
    const pugi::xml_node& node,
    const WhereExpr* expr,
    size_t parentDepth
) {
    if (!expr) {
        return true; // No condition means all pass
    }

    // Try to cast to WhereCondition
    if (const auto* condition = dynamic_cast<const WhereCondition*>(expr)) {
        return evaluateCondition(node, *condition, parentDepth);
    }

    // Try to cast to WhereLogical
    if (const auto* logical = dynamic_cast<const WhereLogical*>(expr)) {
        bool leftResult = evaluateWhereExpr(node, logical->left.get(), parentDepth);
        bool rightResult = evaluateWhereExpr(node, logical->right.get(), parentDepth);

        switch (logical->op) {
            case LogicalOp::AND:
                return leftResult && rightResult;
            case LogicalOp::OR:
                return leftResult || rightResult;
            default:
                return false;
        }
    }

    return false; // Unknown expression type
}

bool XmlNavigator::evaluateCondition(
    const pugi::xml_node& node,
    const WhereCondition& condition
) {
    // Special handling for IS NULL and IS NOT NULL
    if (condition.op == ComparisonOp::IS_NULL) {
        // IS NULL: true if attribute is NOT present (nodeValue is empty)
        std::string nodeValue = getNodeValue(node, condition.field);
        return nodeValue.empty();
    }

    if (condition.op == ComparisonOp::IS_NOT_NULL) {
        // IS NOT NULL: true if attribute IS present (nodeValue is not empty)
        std::string nodeValue = getNodeValue(node, condition.field);
        return !nodeValue.empty();
    }

    // Special handling for IN and NOT_IN
    if (condition.op == ComparisonOp::IN || condition.op == ComparisonOp::NOT_IN) {
        std::string nodeValue = getNodeValue(node, condition.field);
        if (nodeValue.empty()) {
            return false;
        }

        // Check if nodeValue exists in the values list
        bool found = false;
        for (const auto& val : condition.values) {
            if (nodeValue == val) {
                found = true;
                break;
            }
        }

        return (condition.op == ComparisonOp::IN) ? found : !found;
    }

    std::string nodeValue = getNodeValue(node, condition.field);

    if (nodeValue.empty()) {
        return false;
    }

    return compareValues(nodeValue, condition.value, condition.op, condition.is_numeric);
}

bool XmlNavigator::evaluateCondition(
    const pugi::xml_node& node,
    const WhereCondition& condition,
    size_t parentDepth
) {
    // Special handling for IS NULL and IS NOT NULL
    if (condition.op == ComparisonOp::IS_NULL) {
        // IS NULL: true if attribute is NOT present (nodeValue is empty)
        std::string nodeValue = getNodeValueRelative(node, condition.field, parentDepth);
        return nodeValue.empty();
    }

    if (condition.op == ComparisonOp::IS_NOT_NULL) {
        // IS NOT NULL: true if attribute IS present (nodeValue is not empty)
        std::string nodeValue = getNodeValueRelative(node, condition.field, parentDepth);
        return !nodeValue.empty();
    }

    // Special handling for IN and NOT_IN
    if (condition.op == ComparisonOp::IN || condition.op == ComparisonOp::NOT_IN) {
        std::string nodeValue = getNodeValueRelative(node, condition.field, parentDepth);
        if (nodeValue.empty()) {
            return false;
        }

        // Check if nodeValue exists in the values list
        bool found = false;
        for (const auto& val : condition.values) {
            if (nodeValue == val) {
                found = true;
                break;
            }
        }

        return (condition.op == ComparisonOp::IN) ? found : !found;
    }

    std::string nodeValue = getNodeValueRelative(node, condition.field, parentDepth);

    if (nodeValue.empty()) {
        return false;
    }

    return compareValues(nodeValue, condition.value, condition.op, condition.is_numeric);
}

void XmlNavigator::findNodes(
    const pugi::xml_node& node,
    const std::vector<std::string>& path,
    size_t depth,
    std::vector<pugi::xml_node>& results
) {
    if (depth >= path.size()) {
        return;
    }

    const std::string& targetName = path[depth];

    // If this is the last component in the path
    if (depth == path.size() - 1) {
        // Find all child nodes with this name
        for (pugi::xml_node child : node.children(targetName.c_str())) {
            results.push_back(child);
        }
    } else {
        // Recurse into children
        for (pugi::xml_node child : node.children(targetName.c_str())) {
            findNodes(child, path, depth + 1, results);
        }
    }
}

std::string XmlNavigator::getNodeValue(
    const pugi::xml_node& node,
    const FieldPath& field
) {
    // Handle attribute extraction
    if (field.is_attribute) {
        pugi::xml_attribute attr = node.attribute(field.attribute_name.c_str());
        if (attr) {
            return attr.value();
        }
        return "";
    }

    if (field.components.empty()) {
        return "";
    }

    // Shorthand: if only one component, search from current node downward
    if (field.components.size() == 1) {
        pugi::xml_node foundNode = findFirstElementByName(node, field.components[0]);
        if (foundNode) {
            return foundNode.child_value();
        }
        return "";
    }

    // Navigate to the target node using full path
    pugi::xml_node current = node;

    for (const auto& component : field.components) {
        current = current.child(component.c_str());
        if (!current) {
            return "";
        }
    }

    return current.child_value();
}

std::string XmlNavigator::getNodeValueRelative(
    const pugi::xml_node& node,
    const FieldPath& field,
    size_t offset
) {
    // Handle attribute extraction
    if (field.is_attribute) {
        pugi::xml_attribute attr = node.attribute(field.attribute_name.c_str());
        if (attr) {
            return attr.value();
        }
        return "";
    }

    if (field.components.empty() || offset >= field.components.size()) {
        return "";
    }

    // Shorthand: if only one component (after offset), search from current node
    if (field.components.size() == 1 && offset == 0) {
        pugi::xml_node foundNode = findFirstElementByName(node, field.components[0]);
        if (foundNode) {
            return foundNode.child_value();
        }
        return "";
    }

    // Navigate using only the components after 'offset'
    pugi::xml_node current = node;

    for (size_t i = offset; i < field.components.size(); ++i) {
        current = current.child(field.components[i].c_str());
        if (!current) {
            return "";
        }
    }

    return current.child_value();
}

bool XmlNavigator::compareValues(
    const std::string& nodeValue,
    const std::string& targetValue,
    ComparisonOp op,
    bool isNumeric
) {
    // Handle LIKE and NOT_LIKE with regex
    if (op == ComparisonOp::LIKE || op == ComparisonOp::NOT_LIKE) {
        try {
            std::regex pattern(targetValue);
            bool matches = std::regex_search(nodeValue, pattern);
            return (op == ComparisonOp::LIKE) ? matches : !matches;
        } catch (const std::regex_error&) {
            return false; // Invalid regex
        }
    }

    if (isNumeric) {
        try {
            double nodeNum = std::stod(nodeValue);
            double targetNum = std::stod(targetValue);

            switch (op) {
                case ComparisonOp::EQUALS:
                    return nodeNum == targetNum;
                case ComparisonOp::NOT_EQUALS:
                    return nodeNum != targetNum;
                case ComparisonOp::LESS_THAN:
                    return nodeNum < targetNum;
                case ComparisonOp::GREATER_THAN:
                    return nodeNum > targetNum;
                case ComparisonOp::LESS_EQUAL:
                    return nodeNum <= targetNum;
                case ComparisonOp::GREATER_EQUAL:
                    return nodeNum >= targetNum;
                default:
                    return false;
            }
        } catch (...) {
            return false;
        }
    } else {
        // String comparison
        switch (op) {
            case ComparisonOp::EQUALS:
                return nodeValue == targetValue;
            case ComparisonOp::NOT_EQUALS:
                return nodeValue != targetValue;
            case ComparisonOp::LESS_THAN:
                return nodeValue < targetValue;
            case ComparisonOp::GREATER_THAN:
                return nodeValue > targetValue;
            case ComparisonOp::LESS_EQUAL:
                return nodeValue <= targetValue;
            case ComparisonOp::GREATER_EQUAL:
                return nodeValue >= targetValue;
            default:
                return false;
        }
    }

    return false;
}

pugi::xml_node XmlNavigator::findFirstElementByName(
    const pugi::xml_node& node,
    const std::string& name
) {
    // Check if current node matches
    if (node && std::string(node.name()) == name) {
        return node;
    }

    // Depth-first search through children
    for (pugi::xml_node child : node.children()) {
        pugi::xml_node found = findFirstElementByName(child, name);
        if (found) {
            return found;
        }
    }

    // Not found
    return pugi::xml_node();
}

} // namespace expocli
