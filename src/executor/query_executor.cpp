#include "executor/query_executor.h"
#include "utils/xml_loader.h"
#include <filesystem>
#include <iostream>

namespace xmlquery {

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
                if (field.include_filename) {
                    fieldName = "FILE_NAME";
                } else {
                    fieldName = field.components.back();
                }

                if (i < fr.size()) {
                    row[fieldName] = fr[i].value;
                } else {
                    row[fieldName] = "";
                }
            }
            results.push_back(row);
        }
    } else {
        // Process with WHERE clause
        // We need to find nodes that match the WHERE condition

        // Get the root path for traversal (parent path of WHERE field)
        const auto& whereField = query.where->field;

        if (whereField.components.size() < 2) {
            // Simple case: condition on root-level elements
            return results;
        }

        // Navigate to parent nodes that contain the WHERE field
        std::vector<std::string> parentPath(
            whereField.components.begin(),
            whereField.components.end() - 1
        );

        std::vector<pugi::xml_node> candidateNodes;
        XmlNavigator::findNodes(*doc, parentPath, 0, candidateNodes);

        // Filter nodes based on WHERE condition
        // Pass parentPath.size() so evaluateCondition uses relative path navigation
        for (const auto& node : candidateNodes) {
            if (XmlNavigator::evaluateCondition(node, *query.where, parentPath.size())) {
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

                    row[fieldName] = value;
                }

                results.push_back(row);
            }
        }
    }

    return results;
}

} // namespace xmlquery
