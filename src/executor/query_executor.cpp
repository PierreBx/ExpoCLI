#include "executor/query_executor.h"
#include "utils/xml_loader.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <limits>
#include <set>

namespace expocli {

// Helper to extract a FieldPath from any WhereExpr (gets the first condition's field)
static FieldPath extractFieldPathFromWhere(const WhereExpr* expr) {
    if (!expr) {
        return FieldPath();
    }

    // If it's a simple condition, return its field
    if (const auto* condition = dynamic_cast<const WhereCondition*>(expr)) {
        return condition->field;
    }

    // If it's a logical expression, recursively get from left side
    if (const auto* logical = dynamic_cast<const WhereLogical*>(expr)) {
        return extractFieldPathFromWhere(logical->left.get());
    }

    return FieldPath();
}

std::vector<ResultRow> QueryExecutor::execute(const Query& query) {
    std::vector<ResultRow> allResults;

    // Get all XML files from the directory
    std::vector<std::string> xmlFiles = getXmlFiles(query.from_path);

    if (xmlFiles.empty()) {
        std::cerr << "Warning: No XML files found in " << query.from_path << std::endl;
        return allResults;
    }

    // Check if any aggregate functions are used
    bool hasAggregates = false;
    for (const auto& field : query.select_fields) {
        if (field.aggregate != AggregateFunc::NONE) {
            hasAggregates = true;
            break;
        }
    }

    // Process each file - for aggregates, we need to build a modified query
    if (hasAggregates) {
        // For aggregate queries, build a temporary query to extract fields
        Query tempQuery;
        tempQuery.from_path = query.from_path;
        tempQuery.where = nullptr;  // We'll handle WHERE separately for now
        tempQuery.distinct = false;
        tempQuery.limit = -1;
        tempQuery.offset = -1;

        // Convert aggregate fields to regular fields for extraction
        for (const auto& field : query.select_fields) {
            if (field.aggregate != AggregateFunc::NONE && !field.is_count_star) {
                // Extract the underlying field for aggregation
                FieldPath extractField = field;
                extractField.aggregate = AggregateFunc::NONE;
                tempQuery.select_fields.push_back(extractField);
            }
        }

        // For COUNT(*) with no other fields, we need at least one field to process
        // We'll count based on file loading success
        bool isOnlyCountStar = tempQuery.select_fields.empty();

        if (!isOnlyCountStar) {
            // Process files to extract field values
            for (const auto& filepath : xmlFiles) {
                try {
                    auto fileResults = processFile(filepath, tempQuery);
                    allResults.insert(allResults.end(), fileResults.begin(), fileResults.end());
                } catch (const std::exception& e) {
                    std::cerr << "Error processing file " << filepath << ": " << e.what() << std::endl;
                }
            }
        } else {
            // For COUNT(*) only, count files as rows
            // For now, without WHERE clause support in pure COUNT(*), just count files
            for (const auto& filepath : xmlFiles) {
                try {
                    auto doc = XmlLoader::load(filepath);
                    // Each successfully loaded file counts as a row for COUNT(*)
                    allResults.push_back(ResultRow());
                } catch (const std::exception& e) {
                    std::cerr << "Error processing file " << filepath << ": " << e.what() << std::endl;
                }
            }
        }

        // Now compute aggregates
        ResultRow aggregateRow;
        for (const auto& field : query.select_fields) {
            std::string fieldName;
            if (field.is_count_star) {
                fieldName = "COUNT(*)";
            } else {
                std::string path;
                if (field.is_attribute) {
                    path = "@" + field.attribute_name;
                } else {
                    for (size_t i = 0; i < field.components.size(); ++i) {
                        if (i > 0) path += ".";
                        path += field.components[i];
                    }
                }

                switch (field.aggregate) {
                    case AggregateFunc::COUNT: fieldName = "COUNT(" + path + ")"; break;
                    case AggregateFunc::SUM:   fieldName = "SUM(" + path + ")"; break;
                    case AggregateFunc::AVG:   fieldName = "AVG(" + path + ")"; break;
                    case AggregateFunc::MIN:   fieldName = "MIN(" + path + ")"; break;
                    case AggregateFunc::MAX:   fieldName = "MAX(" + path + ")"; break;
                    default: break;
                }
            }

            std::string aggregateValue = computeAggregate(field, allResults);
            aggregateRow.push_back({fieldName, aggregateValue});
        }

        return {aggregateRow};
    }

    // Non-aggregate query - process normally
    for (const auto& filepath : xmlFiles) {
        try {
            auto fileResults = processFile(filepath, query);
            allResults.insert(allResults.end(), fileResults.begin(), fileResults.end());
        } catch (const std::exception& e) {
            std::cerr << "Error processing file " << filepath << ": " << e.what() << std::endl;
        }
    }

    // Apply ORDER BY if specified
    if (!query.order_by_fields.empty()) {
        const OrderByField& orderByField = query.order_by_fields[0]; // For now, support first field only
        const std::string& orderField = orderByField.field_name;
        bool descending = (orderByField.direction == SortDirection::DESC);

        std::sort(allResults.begin(), allResults.end(),
            [&orderField, descending](const ResultRow& a, const ResultRow& b) {
                // Find the field in both rows
                std::string aValue, bValue;

                for (const auto& [field, value] : a) {
                    if (field == orderField) {
                        aValue = value;
                        break;
                    }
                }

                for (const auto& [field, value] : b) {
                    if (field == orderField) {
                        bValue = value;
                        break;
                    }
                }

                // Try numeric comparison first
                try {
                    double aNum = std::stod(aValue);
                    double bNum = std::stod(bValue);
                    // For descending, we want larger values first (a > b means a before b)
                    // For ascending, we want smaller values first (a < b means a before b)
                    return descending ? (aNum > bNum) : (aNum < bNum);
                } catch (...) {
                    // Fall back to string comparison
                    return descending ? (aValue > bValue) : (aValue < bValue);
                }
            }
        );
    }

    // Apply DISTINCT if specified (remove duplicate rows)
    if (query.distinct) {
        std::vector<ResultRow> uniqueResults;
        std::set<std::string> seen;  // Store serialized rows for comparison

        for (const auto& row : allResults) {
            // Serialize the row for comparison
            std::string rowKey;
            for (const auto& [field, value] : row) {
                rowKey += field + ":" + value + "|";
            }

            // Only add if we haven't seen this row before
            if (seen.find(rowKey) == seen.end()) {
                seen.insert(rowKey);
                uniqueResults.push_back(row);
            }
        }

        allResults = std::move(uniqueResults);
    }

    // Apply OFFSET if specified (skip first N results)
    if (query.offset >= 0 && static_cast<size_t>(query.offset) < allResults.size()) {
        allResults.erase(allResults.begin(), allResults.begin() + query.offset);
    } else if (query.offset >= 0 && static_cast<size_t>(query.offset) >= allResults.size()) {
        // Offset is beyond the result set, return empty
        allResults.clear();
    }

    // Apply LIMIT if specified (after offset)
    if (query.limit >= 0 && static_cast<size_t>(query.limit) < allResults.size()) {
        allResults.resize(query.limit);
    }

    return allResults;
}

std::vector<std::string> QueryExecutor::getXmlFiles(const std::string& path) {
    std::vector<std::string> xmlFiles;

    try {
        if (std::filesystem::is_regular_file(path)) {
            // Single file
            if (XmlLoader::isXmlFile(path)) {
                xmlFiles.push_back(path);
            }
        } else if (std::filesystem::is_directory(path)) {
            // Directory - scan for XML files
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if (entry.is_regular_file() && XmlLoader::isXmlFile(entry.path().string())) {
                    xmlFiles.push_back(entry.path().string());
                }
            }
        } else {
            std::cerr << "Warning: Path is neither a file nor a directory: " << path << std::endl;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }

    return xmlFiles;
}

// Process a single file with FOR clause context binding
std::vector<ResultRow> QueryExecutor::processFileWithForClauses(
    const std::string& filepath,
    const Query& query,
    const pugi::xml_document& doc,
    const std::string& filename
) {
    std::vector<ResultRow> results;

    if (query.for_clauses.empty()) {
        return results;
    }

    // Variable context: maps variable name -> bound XML node
    std::map<std::string, pugi::xml_node> varContext;

    // Position context: maps position variable name -> current position
    std::map<std::string, size_t> positionContext;

    // Start nested iteration from document root
    processNestedForClauses(doc.document_element(), query, varContext, positionContext, 0, filename, results);

    // If query has aggregations, apply aggregation logic
    if (query.has_aggregates && !results.empty()) {
        std::vector<ResultRow> aggregatedResults;

        // For now, assume no GROUP BY - aggregate all results into a single row
        // TODO: Add GROUP BY support
        if (query.group_by_fields.empty()) {
            ResultRow aggregatedRow;

            // Process each SELECT field
            for (const auto& field : query.select_fields) {
                if (field.aggregate != AggregateFunc::NONE) {
                    std::string fieldName = field.alias.empty() ?
                        (std::string(field.aggregate == AggregateFunc::COUNT ? "COUNT" :
                                    field.aggregate == AggregateFunc::SUM ? "SUM" :
                                    field.aggregate == AggregateFunc::AVG ? "AVG" :
                                    field.aggregate == AggregateFunc::MIN ? "MIN" : "MAX") +
                         "(" + field.aggregate_arg + ")") : field.alias;

                    // Find this field in the results and aggregate
                    std::vector<std::string> values;
                    for (const auto& row : results) {
                        for (const auto& [name, val] : row) {
                            if (name == fieldName || name.find(field.aggregate_arg) != std::string::npos) {
                                values.push_back(val);
                                break;
                            }
                        }
                    }

                    // Apply aggregation function
                    std::string aggregatedValue;
                    switch (field.aggregate) {
                        case AggregateFunc::COUNT:
                            aggregatedValue = std::to_string(values.size());
                            break;
                        case AggregateFunc::SUM: {
                            double sum = 0;
                            for (const auto& v : values) {
                                try {
                                    sum += std::stod(v);
                                } catch (...) {
                                    // Skip non-numeric values
                                }
                            }
                            aggregatedValue = std::to_string(sum);
                            break;
                        }
                        case AggregateFunc::AVG: {
                            double sum = 0;
                            size_t count = 0;
                            for (const auto& v : values) {
                                try {
                                    sum += std::stod(v);
                                    count++;
                                } catch (...) {
                                    // Skip non-numeric values
                                }
                            }
                            aggregatedValue = count > 0 ? std::to_string(sum / count) : "0";
                            break;
                        }
                        case AggregateFunc::MIN: {
                            double minVal = std::numeric_limits<double>::max();
                            for (const auto& v : values) {
                                try {
                                    double val = std::stod(v);
                                    if (val < minVal) minVal = val;
                                } catch (...) {
                                    // Skip non-numeric values
                                }
                            }
                            aggregatedValue = minVal == std::numeric_limits<double>::max() ? "0" : std::to_string(minVal);
                            break;
                        }
                        case AggregateFunc::MAX: {
                            double maxVal = std::numeric_limits<double>::lowest();
                            for (const auto& v : values) {
                                try {
                                    double val = std::stod(v);
                                    if (val > maxVal) maxVal = val;
                                } catch (...) {
                                    // Skip non-numeric values
                                }
                            }
                            aggregatedValue = maxVal == std::numeric_limits<double>::lowest() ? "0" : std::to_string(maxVal);
                            break;
                        }
                        default:
                            aggregatedValue = "0";
                    }

                    aggregatedRow.push_back({fieldName, aggregatedValue});
                }
            }

            aggregatedResults.push_back(aggregatedRow);
            return aggregatedResults;
        } else {
            // Handle GROUP BY aggregations
            // Group results by GROUP BY field values
            std::map<std::string, std::vector<ResultRow>> groups;

            for (const auto& row : results) {
                // Build group key from GROUP BY fields
                std::string groupKey;
                for (const auto& groupField : query.group_by_fields) {
                    std::string groupByFieldName = "__GROUP_BY__" + groupField;
                    for (const auto& [name, val] : row) {
                        if (name == groupByFieldName) {
                            if (!groupKey.empty()) groupKey += "|||";
                            groupKey += val;
                            break;
                        }
                    }
                }
                groups[groupKey].push_back(row);
            }

            // For each group, compute aggregations
            for (const auto& [groupKey, groupRows] : groups) {
                ResultRow aggregatedRow;

                // First, add the GROUP BY field values to the result
                size_t groupFieldIndex = 0;
                std::string remainingKey = groupKey;
                for (const auto& groupField : query.group_by_fields) {
                    // Extract value from group key
                    std::string groupValue;
                    size_t pos = remainingKey.find("|||");
                    if (pos != std::string::npos) {
                        groupValue = remainingKey.substr(0, pos);
                        remainingKey = remainingKey.substr(pos + 3);
                    } else {
                        groupValue = remainingKey;
                    }

                    // Add to result row (use the field name directly, not the __GROUP_BY__ prefix)
                    aggregatedRow.push_back({groupField, groupValue});
                    groupFieldIndex++;
                }

                // Process each SELECT field
                for (const auto& field : query.select_fields) {
                    if (field.aggregate != AggregateFunc::NONE) {
                        std::string fieldName = field.alias.empty() ?
                            (std::string(field.aggregate == AggregateFunc::COUNT ? "COUNT" :
                                        field.aggregate == AggregateFunc::SUM ? "SUM" :
                                        field.aggregate == AggregateFunc::AVG ? "AVG" :
                                        field.aggregate == AggregateFunc::MIN ? "MIN" : "MAX") +
                             "(" + field.aggregate_arg + ")") : field.alias;

                        // Collect values from this group
                        std::vector<std::string> values;
                        for (const auto& row : groupRows) {
                            for (const auto& [name, val] : row) {
                                // Match field name, excluding __GROUP_BY__ fields
                                if (name.find("__GROUP_BY__") != 0 &&
                                    (name == fieldName || name.find(field.aggregate_arg) != std::string::npos)) {
                                    values.push_back(val);
                                    break;
                                }
                            }
                        }

                        // Apply aggregation function
                        std::string aggregatedValue;
                        switch (field.aggregate) {
                            case AggregateFunc::COUNT:
                                aggregatedValue = std::to_string(values.size());
                                break;
                            case AggregateFunc::SUM: {
                                double sum = 0;
                                for (const auto& v : values) {
                                    try {
                                        sum += std::stod(v);
                                    } catch (...) {}
                                }
                                aggregatedValue = std::to_string(sum);
                                break;
                            }
                            case AggregateFunc::AVG: {
                                double sum = 0;
                                size_t count = 0;
                                for (const auto& v : values) {
                                    try {
                                        sum += std::stod(v);
                                        count++;
                                    } catch (...) {}
                                }
                                aggregatedValue = count > 0 ? std::to_string(sum / count) : "0";
                                break;
                            }
                            case AggregateFunc::MIN: {
                                double minVal = std::numeric_limits<double>::max();
                                for (const auto& v : values) {
                                    try {
                                        double val = std::stod(v);
                                        if (val < minVal) minVal = val;
                                    } catch (...) {}
                                }
                                aggregatedValue = minVal == std::numeric_limits<double>::max() ? "0" : std::to_string(minVal);
                                break;
                            }
                            case AggregateFunc::MAX: {
                                double maxVal = std::numeric_limits<double>::lowest();
                                for (const auto& v : values) {
                                    try {
                                        double val = std::stod(v);
                                        if (val > maxVal) maxVal = val;
                                    } catch (...) {}
                                }
                                aggregatedValue = maxVal == std::numeric_limits<double>::lowest() ? "0" : std::to_string(maxVal);
                                break;
                            }
                            default:
                                aggregatedValue = "0";
                        }

                        aggregatedRow.push_back({fieldName, aggregatedValue});
                    }
                }

                aggregatedResults.push_back(aggregatedRow);
            }

            return aggregatedResults;
        }
    }

    return results;
}

// Recursive function to handle nested FOR clauses
void QueryExecutor::processNestedForClauses(
    const pugi::xml_node& currentContext,
    const Query& query,
    std::map<std::string, pugi::xml_node>& varContext,
    std::map<std::string, size_t>& positionContext,
    size_t forClauseIndex,
    const std::string& filename,
    std::vector<ResultRow>& results
) {
    // Base case: all FOR clauses processed, now extract SELECT fields
    if (forClauseIndex >= query.for_clauses.size()) {
        // Check WHERE clause if present
        if (query.where) {
            if (!evaluateWhereWithContext(varContext, positionContext, query.where.get(), query)) {
                return; // Skip this combination if WHERE fails
            }
        }

        // Extract SELECT fields using variable context
        ResultRow row;

        // If we have GROUP BY, also include GROUP BY fields in the row for grouping
        // These will be used to group results before aggregation
        if (query.has_aggregates && !query.group_by_fields.empty()) {
            for (const auto& groupField : query.group_by_fields) {
                // Resolve the group by field value
                FieldPath groupPath;
                // Parse the group field (could be simple or dotted like dept.name)
                std::string component;
                for (char c : groupField) {
                    if (c == '.') {
                        if (!component.empty()) {
                            groupPath.components.push_back(component);
                            component.clear();
                        }
                    } else {
                        component += c;
                    }
                }
                if (!component.empty()) {
                    groupPath.components.push_back(component);
                }

                // Check if it's a variable reference
                if (!groupPath.components.empty() && query.isForVariable(groupPath.components[0])) {
                    groupPath.is_variable_ref = true;
                    groupPath.variable_name = groupPath.components[0];
                }

                std::string groupValue = resolveFieldWithContext(groupPath, varContext, positionContext, currentContext, query);
                row.push_back({"__GROUP_BY__" + groupField, groupValue});
            }
        }

        for (const auto& field : query.select_fields) {
            std::string fieldName;
            std::string value;

            // Handle aggregation functions
            if (field.aggregate != AggregateFunc::NONE) {
                // For aggregations, we'll use special field names and values
                // The actual aggregation computation happens later
                switch (field.aggregate) {
                    case AggregateFunc::COUNT:
                        fieldName = field.alias.empty() ? ("COUNT(" + field.aggregate_arg + ")") : field.alias;
                        value = "1"; // Each iteration contributes 1 to the count
                        break;
                    case AggregateFunc::SUM:
                    case AggregateFunc::AVG:
                    case AggregateFunc::MIN:
                    case AggregateFunc::MAX: {
                        fieldName = field.alias.empty() ?
                            (std::string(field.aggregate == AggregateFunc::SUM ? "SUM" :
                                        field.aggregate == AggregateFunc::AVG ? "AVG" :
                                        field.aggregate == AggregateFunc::MIN ? "MIN" : "MAX") +
                             "(" + field.aggregate_arg + ")") : field.alias;

                        // Parse the aggregate_arg which could be:
                        // - "emp" (variable)
                        // - "emp.salary" (variable.field)
                        // - "salary" (field relative to current context)

                        // Split by dot to get components
                        std::vector<std::string> argComponents;
                        std::string component;
                        for (char c : field.aggregate_arg) {
                            if (c == '.') {
                                if (!component.empty()) {
                                    argComponents.push_back(component);
                                    component.clear();
                                }
                            } else {
                                component += c;
                            }
                        }
                        if (!component.empty()) {
                            argComponents.push_back(component);
                        }

                        // Resolve the value
                        if (!argComponents.empty()) {
                            // Check if first component is a variable
                            if (varContext.find(argComponents[0]) != varContext.end()) {
                                pugi::xml_node varNode = varContext[argComponents[0]];
                                if (argComponents.size() == 1) {
                                    // Just the variable - get its text value
                                    value = varNode.child_value();
                                } else {
                                    // Variable.field - navigate to the field
                                    FieldPath argPath;
                                    argPath.components = std::vector<std::string>(argComponents.begin() + 1, argComponents.end());
                                    argPath.is_variable_ref = true;
                                    argPath.variable_name = argComponents[0];

                                    // Navigate from the variable node
                                    for (const auto& comp : argPath.components) {
                                        pugi::xml_node child = varNode.child(comp.c_str());
                                        if (child) {
                                            value = child.child_value();
                                            break;
                                        }
                                    }
                                }
                            } else {
                                // Not a variable - resolve as field path from current context
                                FieldPath argPath;
                                argPath.components = argComponents;
                                value = resolveFieldWithContext(argPath, varContext, positionContext, currentContext, query);
                            }
                        }
                        break;
                    }
                    default:
                        value = "";
                }
            } else if (field.include_filename) {
                fieldName = "FILE_NAME";
                value = filename;
            } else {
                fieldName = field.components.back();

                // Resolve field using variable context and position context
                value = resolveFieldWithContext(field, varContext, positionContext, currentContext, query);
            }

            row.push_back({fieldName, value});
        }

        results.push_back(row);
        return;
    }

    // Process current FOR clause
    const ForClause& forClause = query.for_clauses[forClauseIndex];

    // Find nodes to iterate over
    std::vector<pugi::xml_node> iterationNodes;

    // Check if FOR path starts with a variable reference
    if (!forClause.path.components.empty()) {
        std::string firstComponent = forClause.path.components[0];

        // Check if it's a variable reference
        auto varIt = varContext.find(firstComponent);
        if (varIt != varContext.end()) {
            // Path is relative to a bound variable (e.g., "dept.employee")
            pugi::xml_node parentNode = varIt->second;

            // Get remaining path components (skip variable name)
            std::vector<std::string> subPath(
                forClause.path.components.begin() + 1,
                forClause.path.components.end()
            );

            if (subPath.size() == 1) {
                // Simple child search
                std::function<void(const pugi::xml_node&)> findElements =
                    [&](const pugi::xml_node& node) {
                        if (node.type() == pugi::node_element && node.name() == subPath[0]) {
                            iterationNodes.push_back(node);
                        }
                        for (pugi::xml_node child : node.children()) {
                            findElements(child);
                        }
                    };
                findElements(parentNode);
            } else if (!subPath.empty()) {
                // Multi-component path from parent node
                XmlNavigator::findNodesByPartialPath(parentNode, subPath, iterationNodes);
            }
        } else {
            // Not a variable reference - search from current context or document root
            if (forClause.path.components.size() == 1) {
                // Simple path: find all matching elements
                std::string elementName = forClause.path.components[0];
                std::function<void(const pugi::xml_node&)> findElements =
                    [&](const pugi::xml_node& node) {
                        if (node.type() == pugi::node_element && node.name() == elementName) {
                            iterationNodes.push_back(node);
                        }
                        for (pugi::xml_node child : node.children()) {
                            findElements(child);
                        }
                    };
                findElements(currentContext.parent() ? currentContext.root() : currentContext);
            } else {
                // Multi-component path
                pugi::xml_node searchRoot = currentContext.parent() ? currentContext.root() : currentContext;
                XmlNavigator::findNodesByPartialPath(searchRoot, forClause.path.components, iterationNodes);
            }
        }
    }

    // Iterate over found nodes and recursively process next FOR clause
    size_t position = 1;  // XQuery positions start at 1
    for (const auto& node : iterationNodes) {
        // Bind this node to the variable
        varContext[forClause.variable] = node;

        // Bind position if AT clause present
        if (forClause.has_position) {
            positionContext[forClause.position_var] = position;
        }

        // Recursively process next FOR clause
        processNestedForClauses(node, query, varContext, positionContext, forClauseIndex + 1, filename, results);

        // Unbind variable (cleanup for next iteration)
        varContext.erase(forClause.variable);
        if (forClause.has_position) {
            positionContext.erase(forClause.position_var);
        }

        position++;
    }
}

// Resolve field value using variable context
std::string QueryExecutor::resolveFieldWithContext(
    const FieldPath& field,
    const std::map<std::string, pugi::xml_node>& varContext,
    const std::map<std::string, size_t>& positionContext,
    const pugi::xml_node& fallbackContext,
    const Query& query
) {
    std::string value;

    // Check if this is a position variable reference
    if (field.is_variable_ref && !field.variable_name.empty() && query.isPositionVariable(field.variable_name)) {
        auto posIt = positionContext.find(field.variable_name);
        if (posIt != positionContext.end()) {
            return std::to_string(posIt->second);
        }
        return "";  // Position variable not found
    }

    if (field.is_variable_ref && !field.variable_name.empty()) {
        // Field starts with a variable reference (e.g., "emp.name")
        auto varIt = varContext.find(field.variable_name);
        if (varIt != varContext.end()) {
            pugi::xml_node contextNode = varIt->second;

            // Get remaining path components after variable name
            std::vector<std::string> subPath;
            if (field.components.size() > 1) {
                subPath.assign(field.components.begin() + 1, field.components.end());
            }

            if (subPath.empty()) {
                // Just the variable node itself - shouldn't happen but handle it
                value = contextNode.child_value();
            } else if (subPath.size() == 1) {
                // Simple child lookup
                pugi::xml_node childNode = XmlNavigator::findFirstElementByName(contextNode, subPath[0]);
                if (childNode) {
                    value = childNode.child_value();
                }
            } else {
                // Multi-component path from variable node
                std::vector<pugi::xml_node> fieldNodes;
                XmlNavigator::findNodesByPartialPath(contextNode, subPath, fieldNodes);
                if (!fieldNodes.empty()) {
                    value = fieldNodes[0].child_value();
                }
            }
        }
    } else {
        // Normal field (not a variable reference) - use fallback context
        if (field.components.size() == 1) {
            pugi::xml_node foundNode = XmlNavigator::findFirstElementByName(fallbackContext, field.components[0]);
            if (foundNode) {
                value = foundNode.child_value();
            }
        } else {
            std::vector<pugi::xml_node> fieldNodes;
            XmlNavigator::findNodesByPartialPath(fallbackContext, field.components, fieldNodes);
            if (!fieldNodes.empty()) {
                value = fieldNodes[0].child_value();
            }
        }
    }

    return value;
}

// Evaluate WHERE expression with variable context
bool QueryExecutor::evaluateWhereWithContext(
    const std::map<std::string, pugi::xml_node>& varContext,
    const std::map<std::string, size_t>& positionContext,
    const WhereExpr* expr,
    const Query& query
) {
    if (!expr) return true;

    if (const auto* condition = dynamic_cast<const WhereCondition*>(expr)) {
        // Check if this is a position variable in WHERE clause
        if (condition->field.is_variable_ref && !condition->field.variable_name.empty() &&
            query.isPositionVariable(condition->field.variable_name)) {
            // Evaluate condition on position value
            auto posIt = positionContext.find(condition->field.variable_name);
            if (posIt != positionContext.end()) {
                size_t posValue = posIt->second;
                std::string posStr = std::to_string(posValue);

                // Evaluate the comparison
                if (condition->op == ComparisonOp::EQUALS) {
                    return posStr == condition->value;
                } else if (condition->op == ComparisonOp::NOT_EQUALS) {
                    return posStr != condition->value;
                } else if (condition->op == ComparisonOp::LESS_THAN) {
                    try {
                        return posValue < std::stoull(condition->value);
                    } catch (...) { return false; }
                } else if (condition->op == ComparisonOp::GREATER_THAN) {
                    try {
                        return posValue > std::stoull(condition->value);
                    } catch (...) { return false; }
                } else if (condition->op == ComparisonOp::LESS_EQUAL) {
                    try {
                        return posValue <= std::stoull(condition->value);
                    } catch (...) { return false; }
                } else if (condition->op == ComparisonOp::GREATER_EQUAL) {
                    try {
                        return posValue >= std::stoull(condition->value);
                    } catch (...) { return false; }
                }
            }
            return false;
        }

        // Resolve field in condition
        if (condition->field.is_variable_ref && !condition->field.variable_name.empty()) {
            // Use variable context
            auto varIt = varContext.find(condition->field.variable_name);
            if (varIt != varContext.end()) {
                pugi::xml_node contextNode = varIt->second;

                // Evaluate condition on this node
                // Need to adjust the field path to be relative to the bound node
                WhereCondition adjustedCondition = *condition;

                // Remove variable name from components
                if (adjustedCondition.field.components.size() > 1) {
                    adjustedCondition.field.components.erase(adjustedCondition.field.components.begin());
                } else {
                    // Condition is on the variable node itself
                    adjustedCondition.field.components.clear();
                }

                return XmlNavigator::evaluateCondition(contextNode, adjustedCondition, 0);
            }
            return false; // Variable not found
        } else {
            // No variable reference - this shouldn't happen with FOR clauses but handle it
            // Use the last bound variable's context if available
            if (!varContext.empty()) {
                return XmlNavigator::evaluateCondition(varContext.rbegin()->second, *condition, 0);
            }
            return false;
        }
    } else if (const auto* logical = dynamic_cast<const WhereLogical*>(expr)) {
        bool leftResult = evaluateWhereWithContext(varContext, positionContext, logical->left.get(), query);
        bool rightResult = evaluateWhereWithContext(varContext, positionContext, logical->right.get(), query);

        if (logical->op == LogicalOp::AND) {
            return leftResult && rightResult;
        } else if (logical->op == LogicalOp::OR) {
            return leftResult || rightResult;
        }
    }

    return true;
}

std::vector<ResultRow> QueryExecutor::processFile(
    const std::string& filepath,
    const Query& query
) {
    std::vector<ResultRow> results;

    // Load the XML document
    auto doc = XmlLoader::load(filepath);

    // Get filename for FILE_NAME field
    std::string filename = std::filesystem::path(filepath).filename().string();

    // Check if query has FOR clauses
    if (!query.for_clauses.empty()) {
        // Process query with FOR clause context binding
        results = processFileWithForClauses(filepath, query, *doc, filename);
        return results;
    }

    // If there's no WHERE clause, extract all values
    if (!query.where) {
        // For each select field, extract all matching values
        std::vector<std::vector<XmlResult>> fieldResults;

        for (const auto& field : query.select_fields) {
            auto values = XmlNavigator::extractValues(*doc, filename, field);
            fieldResults.push_back(values);
        }

        // Combine results
        // For MVP, we'll take the cross product of all field values
        if (fieldResults.empty()) {
            return results;
        }

        // Find the maximum number of results
        size_t maxResults = 0;
        for (const auto& fr : fieldResults) {
            maxResults = std::max(maxResults, fr.size());
        }

        // Create result rows
        for (size_t i = 0; i < maxResults; ++i) {
            ResultRow row;
            for (size_t fieldIdx = 0; fieldIdx < query.select_fields.size(); ++fieldIdx) {
                const auto& field = query.select_fields[fieldIdx];
                const auto& fr = fieldResults[fieldIdx];

                std::string fieldName;
                std::string fieldValue;

                if (field.include_filename) {
                    fieldName = "FILE_NAME";
                } else if (field.is_attribute) {
                    fieldName = "@" + field.attribute_name;
                } else {
                    fieldName = field.components.back();
                }

                if (i < fr.size()) {
                    fieldValue = fr[i].value;
                } else {
                    fieldValue = "";
                }

                row.push_back({fieldName, fieldValue});
            }
            results.push_back(row);
        }
    } else {
        // Process with WHERE clause
        // We need to find nodes that match the WHERE condition

        // Get the root path for traversal (parent path of WHERE field)
        // Extract field from the first condition in the WHERE expression tree
        FieldPath whereField = extractFieldPathFromWhere(query.where.get());

        if (whereField.components.size() < 2) {
            // Shorthand path: find all nodes that contain the WHERE attribute
            // and evaluate the condition on parent nodes that have the attribute as a child

            // Check if this is an IS NULL or IS NOT NULL condition
            bool isNullCheck = false;
            if (const auto* condition = dynamic_cast<const WhereCondition*>(query.where.get())) {
                isNullCheck = (condition->op == ComparisonOp::IS_NULL ||
                              condition->op == ComparisonOp::IS_NOT_NULL);
            }

            std::function<void(const pugi::xml_node&)> searchTree =
                [&](const pugi::xml_node& node) {
                    if (!node) return;

                    // For IS NULL/IS NOT NULL, check all nodes
                    // For other operators, only check nodes that have the attribute
                    bool shouldEvaluate = false;

                    if (isNullCheck) {
                        // For IS NULL/IS NOT NULL, evaluate on nodes that have at least one SELECT field
                        // This ensures we're checking the right "level" of nodes
                        if (node.type() == pugi::node_element && node != *doc) {
                            // Check if this node has at least one of the SELECT fields as a child or attribute
                            for (const auto& selectField : query.select_fields) {
                                if (!selectField.include_filename) {
                                    if (selectField.is_attribute) {
                                        // For attributes, just check if this is an element node
                                        shouldEvaluate = true;
                                        break;
                                    } else if (selectField.components.size() == 1) {
                                        pugi::xml_node foundNode = XmlNavigator::findFirstElementByName(node, selectField.components[0]);
                                        if (foundNode && foundNode.parent() == node) {
                                            shouldEvaluate = true;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        // Check if this node has the WHERE field
                        if (whereField.is_attribute) {
                            // For attributes, check if this node is an element node
                            // The actual attribute value will be checked in evaluateWhereExpr
                            shouldEvaluate = (node.type() == pugi::node_element && node != *doc);
                        } else if (!whereField.components.empty()) {
                            // Check if this node has the WHERE field as a direct child
                            pugi::xml_node whereAttrNode = XmlNavigator::findFirstElementByName(node, whereField.components[0]);
                            shouldEvaluate = (whereAttrNode && whereAttrNode.parent() == node);
                        }
                    }

                    if (shouldEvaluate) {
                        // Evaluate WHERE condition on this node
                        if (XmlNavigator::evaluateWhereExpr(node, query.where.get(), 0)) {
                            ResultRow row;

                            for (const auto& field : query.select_fields) {
                                std::string fieldName;
                                std::string value;

                                if (field.include_filename) {
                                    fieldName = "FILE_NAME";
                                    value = filename;
                                } else if (field.is_attribute) {
                                    fieldName = "@" + field.attribute_name;
                                    // Extract attribute from current node
                                    pugi::xml_attribute attr = node.attribute(field.attribute_name.c_str());
                                    if (attr) {
                                        value = attr.value();
                                    }
                                } else {
                                    fieldName = field.components.back();

                                    // Use shorthand search from this node
                                    if (field.components.size() == 1) {
                                        pugi::xml_node foundNode = XmlNavigator::findFirstElementByName(node, field.components[0]);
                                        if (foundNode) {
                                            value = foundNode.child_value();
                                        }
                                    } else {
                                        // Use partial path matching from this node
                                        std::vector<pugi::xml_node> fieldNodes;
                                        XmlNavigator::findNodesByPartialPath(node, field.components, fieldNodes);

                                        if (!fieldNodes.empty()) {
                                            value = fieldNodes[0].child_value();
                                        }
                                    }
                                }

                                row.push_back({fieldName, value});
                            }

                            results.push_back(row);
                        }
                    }

                    // Recursively search children
                    for (pugi::xml_node child : node.children()) {
                        searchTree(child);
                    }
                };

            searchTree(*doc);
            return results;
        }

        // Navigate to parent nodes that contain the WHERE field
        // Use partial path matching to find all nodes matching the parent path suffix
        std::vector<std::string> parentPath(
            whereField.components.begin(),
            whereField.components.end() - 1
        );

        std::vector<pugi::xml_node> candidateNodes;
        XmlNavigator::findNodesByPartialPath(*doc, parentPath, candidateNodes);

        // Filter nodes based on WHERE expression
        // Pass parentPath.size() so evaluation uses relative path navigation
        for (const auto& node : candidateNodes) {
            if (XmlNavigator::evaluateWhereExpr(node, query.where.get(), parentPath.size())) {
                // Extract select fields from this node
                ResultRow row;

                for (const auto& field : query.select_fields) {
                    std::string fieldName;
                    std::string value;

                    if (field.include_filename) {
                        fieldName = "FILE_NAME";
                        value = filename;
                    } else if (field.is_attribute) {
                        fieldName = "@" + field.attribute_name;
                        // Extract attribute from current node
                        pugi::xml_attribute attr = node.attribute(field.attribute_name.c_str());
                        if (attr) {
                            value = attr.value();
                        }
                    } else {
                        fieldName = field.components.back();

                        // Shorthand: use first element search
                        if (field.components.size() == 1) {
                            pugi::xml_node foundNode = XmlNavigator::findFirstElementByName(node, field.components[0]);
                            if (foundNode) {
                                value = foundNode.child_value();
                            }
                        } else {
                            // Use partial path matching relative to current node
                            // First, try to find the field using partial path from this node
                            std::vector<pugi::xml_node> fieldNodes;
                            XmlNavigator::findNodesByPartialPath(node, field.components, fieldNodes);

                            if (!fieldNodes.empty()) {
                                // Use the first match
                                value = fieldNodes[0].child_value();
                            }
                        }
                    }

                    row.push_back({fieldName, value});
                }

                results.push_back(row);
            }
        }
    }

    return results;
}

std::vector<std::string> QueryExecutor::checkForAmbiguousAttributes(const Query& query) {
    std::vector<std::string> ambiguousAttrs;

    // Get the first XML file from the query path to analyze structure
    std::vector<std::string> xmlFiles = getXmlFiles(query.from_path);
    if (xmlFiles.empty()) {
        // No files to check
        return ambiguousAttrs;
    }

    // Load the first file as a representative sample
    auto doc = XmlLoader::load(xmlFiles[0]);
    if (!doc) {
        return ambiguousAttrs;
    }

    // Helper to format field path as string
    auto pathToString = [](const FieldPath& field) -> std::string {
        std::string result;
        for (size_t i = 0; i < field.components.size(); ++i) {
            if (i > 0) result += ".";
            result += field.components[i];
        }
        return result;
    };

    // Check SELECT fields (only partial paths with 2+ components can be ambiguous)
    for (const auto& field : query.select_fields) {
        if (field.include_filename) continue; // FILE_NAME is never ambiguous
        if (field.components.size() < 2) continue; // Top-level attributes are never ambiguous

        int matchCount = XmlNavigator::countMatchingPaths(*doc, field.components);
        if (matchCount > 1) {
            ambiguousAttrs.push_back(pathToString(field));
        }
    }

    // Check WHERE clause fields
    if (query.where) {
        std::function<void(const WhereExpr*)> checkWhereFields =
            [&](const WhereExpr* expr) {
                if (!expr) return;

                if (const auto* condition = dynamic_cast<const WhereCondition*>(expr)) {
                    const auto& field = condition->field;
                    if (field.components.size() >= 2) {
                        int matchCount = XmlNavigator::countMatchingPaths(*doc, field.components);
                        if (matchCount > 1) {
                            std::string fieldStr = pathToString(field);
                            // Avoid duplicates
                            if (std::find(ambiguousAttrs.begin(), ambiguousAttrs.end(), fieldStr) == ambiguousAttrs.end()) {
                                ambiguousAttrs.push_back(fieldStr);
                            }
                        }
                    }
                } else if (const auto* logical = dynamic_cast<const WhereLogical*>(expr)) {
                    checkWhereFields(logical->left.get());
                    checkWhereFields(logical->right.get());
                }
            };

        checkWhereFields(query.where.get());
    }

    return ambiguousAttrs;
}

size_t QueryExecutor::getOptimalThreadCount() {
    // Get hardware concurrency (number of logical CPU cores)
    size_t hwThreads = std::thread::hardware_concurrency();

    // If we can't detect, default to 4 threads
    if (hwThreads == 0) {
        hwThreads = 4;
    }

    // Cap at 16 threads to avoid excessive overhead
    return std::min(hwThreads, static_cast<size_t>(16));
}

bool QueryExecutor::shouldUseThreading(size_t fileCount) {
    // Smart threshold calculation:
    // - Single file: never use threading
    // - 2-4 files: not worth the threading overhead
    // - 5+ files: use threading

    size_t threshold = 5;

    // Also consider: if we have fewer files than threads,
    // threading is only beneficial if files are large enough
    // For now, use simple threshold

    return fileCount >= threshold;
}

std::vector<ResultRow> QueryExecutor::executeMultithreaded(
    const std::vector<std::string>& xmlFiles,
    const Query& query,
    size_t threadCount,
    std::atomic<size_t>* completedCounter
) {
    std::vector<ResultRow> allResults;
    std::mutex resultsMutex;

    // Create thread pool
    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    // Atomic counter for completed files (local if not provided)
    std::atomic<size_t> localCompleted{0};
    std::atomic<size_t>* completed = completedCounter ? completedCounter : &localCompleted;

    // Launch worker threads
    for (size_t threadId = 0; threadId < threadCount; ++threadId) {
        threads.emplace_back([&, threadId]() {
            // Each thread processes every Nth file (strided access for load balancing)
            for (size_t fileIdx = threadId; fileIdx < xmlFiles.size(); fileIdx += threadCount) {
                try {
                    // Process this file
                    auto fileResults = processFile(xmlFiles[fileIdx], query);

                    // Accumulate results (thread-safe)
                    {
                        std::lock_guard<std::mutex> lock(resultsMutex);
                        allResults.insert(allResults.end(),
                                        fileResults.begin(),
                                        fileResults.end());
                    }

                    // Increment completed counter
                    (*completed)++;

                } catch (const std::exception& e) {
                    std::cerr << "Error processing file " << xmlFiles[fileIdx]
                              << ": " << e.what() << std::endl;
                    (*completed)++;
                }
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    return allResults;
}

std::vector<ResultRow> QueryExecutor::executeWithProgress(
    const Query& query,
    ProgressCallback progressCallback,
    ExecutionStats* stats
) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Get all XML files
    std::vector<std::string> xmlFiles = getXmlFiles(query.from_path);

    if (xmlFiles.empty()) {
        std::cerr << "Warning: No XML files found in " << query.from_path << std::endl;
        return std::vector<ResultRow>();
    }

    size_t fileCount = xmlFiles.size();
    bool useThreading = shouldUseThreading(fileCount);
    size_t threadCount = useThreading ? getOptimalThreadCount() : 1;

    // Update stats if provided
    if (stats) {
        stats->total_files = fileCount;
        stats->thread_count = threadCount;
        stats->used_threading = useThreading;
    }

    std::vector<ResultRow> allResults;

    if (useThreading) {
        // Multi-threaded execution with progress tracking
        std::atomic<size_t> completed{0};

        // Launch a progress monitoring thread
        std::atomic<bool> done{false};
        std::thread progressThread([&]() {
            while (!done) {
                if (progressCallback) {
                    progressCallback(completed.load(), fileCount, threadCount);
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });

        // Execute query with multi-threading
        allResults = executeMultithreaded(xmlFiles, query, threadCount, &completed);

        // Stop progress thread
        done = true;
        progressThread.join();

        // Final progress update
        if (progressCallback) {
            progressCallback(fileCount, fileCount, threadCount);
        }

    } else {
        // Single-threaded execution (for small file counts)
        for (size_t i = 0; i < xmlFiles.size(); ++i) {
            try {
                auto fileResults = processFile(xmlFiles[i], query);
                allResults.insert(allResults.end(), fileResults.begin(), fileResults.end());

                if (progressCallback) {
                    progressCallback(i + 1, fileCount, 1);
                }
            } catch (const std::exception& e) {
                std::cerr << "Error processing file " << xmlFiles[i] << ": " << e.what() << std::endl;
            }
        }
    }

    // Apply ORDER BY if specified
    if (!query.order_by_fields.empty()) {
        const auto& orderByField = query.order_by_fields[0];
        const std::string& orderField = orderByField.field_name;
        bool ascending = (orderByField.direction == SortDirection::ASC);

        std::sort(allResults.begin(), allResults.end(),
            [&orderField, ascending](const ResultRow& a, const ResultRow& b) {
                std::string aValue, bValue;

                for (const auto& [field, value] : a) {
                    if (field == orderField) {
                        aValue = value;
                        break;
                    }
                }

                for (const auto& [field, value] : b) {
                    if (field == orderField) {
                        bValue = value;
                        break;
                    }
                }

                // Try numeric comparison first
                bool result;
                try {
                    double aNum = std::stod(aValue);
                    double bNum = std::stod(bValue);
                    result = aNum < bNum;
                } catch (...) {
                    result = aValue < bValue;
                }

                return ascending ? result : !result;
            }
        );
    }

    // Apply LIMIT if specified
    if (query.limit >= 0 && static_cast<size_t>(query.limit) < allResults.size()) {
        allResults.resize(query.limit);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;

    if (stats) {
        stats->execution_time_seconds = elapsed.count();
    }

    return allResults;
}

std::string QueryExecutor::computeAggregate(const FieldPath& field, const std::vector<ResultRow>& allResults) {
    if (field.is_count_star) {
        // COUNT(*) - count all rows
        return std::to_string(allResults.size());
    }

    // Build the field name we're looking for
    std::string targetField;
    if (field.is_attribute) {
        targetField = "@" + field.attribute_name;
    } else {
        targetField = field.components.back();
    }

    // Collect all values for this field
    std::vector<double> numericValues;
    size_t count = 0;

    for (const auto& row : allResults) {
        for (const auto& [fieldName, fieldValue] : row) {
            if (fieldName == targetField && !fieldValue.empty()) {
                try {
                    double numValue = std::stod(fieldValue);
                    numericValues.push_back(numValue);
                    count++;
                } catch (...) {
                    // Not a number, skip for SUM/AVG/MIN/MAX but count for COUNT
                    if (field.aggregate == AggregateFunc::COUNT) {
                        count++;
                    }
                }
                break; // Found the field in this row
            }
        }
    }

    switch (field.aggregate) {
        case AggregateFunc::COUNT:
            return std::to_string(count);

        case AggregateFunc::SUM: {
            if (numericValues.empty()) return "0";
            double sum = 0;
            for (double v : numericValues) {
                sum += v;
            }
            return std::to_string(sum);
        }

        case AggregateFunc::AVG: {
            if (numericValues.empty()) return "0";
            double sum = 0;
            for (double v : numericValues) {
                sum += v;
            }
            return std::to_string(sum / numericValues.size());
        }

        case AggregateFunc::MIN: {
            if (numericValues.empty()) return "";
            double minVal = numericValues[0];
            for (double v : numericValues) {
                if (v < minVal) minVal = v;
            }
            return std::to_string(minVal);
        }

        case AggregateFunc::MAX: {
            if (numericValues.empty()) return "";
            double maxVal = numericValues[0];
            for (double v : numericValues) {
                if (v > maxVal) maxVal = v;
            }
            return std::to_string(maxVal);
        }

        default:
            return "";
    }
}

} // namespace expocli
