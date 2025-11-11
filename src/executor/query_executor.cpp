#include "executor/query_executor.h"
#include "utils/xml_loader.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <functional>

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

    // Process each file
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
        const std::string& orderField = query.order_by_fields[0]; // For Phase 2, support first field only

        std::sort(allResults.begin(), allResults.end(),
            [&orderField](const ResultRow& a, const ResultRow& b) {
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
                    return aNum < bNum;
                } catch (...) {
                    // Fall back to string comparison
                    return aValue < bValue;
                }
            }
        );
    }

    // Apply LIMIT if specified
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

} // namespace expocli
