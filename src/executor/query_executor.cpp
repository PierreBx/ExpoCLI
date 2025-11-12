#include "executor/query_executor.h"
#include "utils/xml_loader.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <functional>
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
                for (size_t i = 0; i < field.components.size(); ++i) {
                    if (i > 0) path += ".";
                    path += field.components[i];
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

std::vector<ResultRow> QueryExecutor::processFile(
    const std::string& filepath,
    const Query& query
) {
    std::vector<ResultRow> results;

    // Load the XML document
    auto doc = XmlLoader::load(filepath);

    // Get filename for FILE_NAME field
    std::string filename = std::filesystem::path(filepath).filename().string();

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
                            // Check if this node has at least one of the SELECT fields as a child
                            for (const auto& selectField : query.select_fields) {
                                if (!selectField.include_filename && selectField.components.size() == 1) {
                                    pugi::xml_node foundNode = XmlNavigator::findFirstElementByName(node, selectField.components[0]);
                                    if (foundNode && foundNode.parent() == node) {
                                        shouldEvaluate = true;
                                        break;
                                    }
                                }
                            }
                        }
                    } else {
                        // Check if this node has the WHERE attribute as a direct child
                        pugi::xml_node whereAttrNode = XmlNavigator::findFirstElementByName(node, whereField.components[0]);
                        shouldEvaluate = (whereAttrNode && whereAttrNode.parent() == node);
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
                                } else {
                                    fieldName = field.components.back();

                                    // Use shorthand search from this node
                                    if (field.components.size() == 1) {
                                        pugi::xml_node foundNode = XmlNavigator::findFirstElementByName(node, field.components[0]);
                                        if (foundNode) {
                                            value = foundNode.child_value();
                                        }
                                    } else {
                                        // Use full path navigation
                                        pugi::xml_node targetNode = node;
                                        for (const auto& component : field.components) {
                                            targetNode = targetNode.child(component.c_str());
                                            if (!targetNode) break;
                                        }
                                        if (targetNode) {
                                            value = targetNode.child_value();
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
        std::vector<std::string> parentPath(
            whereField.components.begin(),
            whereField.components.end() - 1
        );

        std::vector<pugi::xml_node> candidateNodes;
        XmlNavigator::findNodes(*doc, parentPath, 0, candidateNodes);

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
                    } else {
                        fieldName = field.components.back();

                        // Shorthand: use first element search
                        if (field.components.size() == 1) {
                            pugi::xml_node foundNode = XmlNavigator::findFirstElementByName(node, field.components[0]);
                            if (foundNode) {
                                value = foundNode.child_value();
                            }
                        } else {
                            // Calculate overlap between field path and parent path
                            // to use relative navigation from the candidate node
                            size_t skipComponents = 0;

                            // Check if field path starts with parent path
                            if (field.components.size() >= parentPath.size()) {
                                bool matches = true;
                                for (size_t i = 0; i < parentPath.size(); ++i) {
                                    if (field.components[i] != parentPath[i]) {
                                        matches = false;
                                        break;
                                    }
                                }
                                if (matches) {
                                    skipComponents = parentPath.size();
                                }
                            }

                            // Navigate from the candidate node using relative path
                            pugi::xml_node targetNode = node;
                            for (size_t i = skipComponents; i < field.components.size(); ++i) {
                                targetNode = targetNode.child(field.components[i].c_str());
                                if (!targetNode) break;
                            }

                            if (targetNode) {
                                value = targetNode.child_value();
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

std::string QueryExecutor::computeAggregate(const FieldPath& field, const std::vector<ResultRow>& allResults) {
    if (field.is_count_star) {
        // COUNT(*) - count all rows
        return std::to_string(allResults.size());
    }

    // Build the field name we're looking for
    std::string targetField = field.components.back();

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
