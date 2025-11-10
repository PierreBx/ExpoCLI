#include "executor/xml_navigator.h"
#include <stdexcept>
#include <typeinfo>

namespace xmlquery {

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

    if (field.components.empty()) {
        return results;
    }

    // Find all nodes matching the path
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
    if (field.components.empty()) {
        return "";
    }

    // Navigate to the target node
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
    if (field.components.empty() || offset >= field.components.size()) {
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
        }
    }

    return false;
}

} // namespace xmlquery
