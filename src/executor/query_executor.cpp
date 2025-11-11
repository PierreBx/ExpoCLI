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

    // Start nested iteration from document root
    processNestedForClauses(doc.document_element(), query, varContext, 0, filename, results);

    return results;
}

// Recursive function to handle nested FOR clauses
void QueryExecutor::processNestedForClauses(
    const pugi::xml_node& currentContext,
    const Query& query,
    std::map<std::string, pugi::xml_node>& varContext,
    size_t forClauseIndex,
    const std::string& filename,
    std::vector<ResultRow>& results
) {
    // Base case: all FOR clauses processed, now extract SELECT fields
    if (forClauseIndex >= query.for_clauses.size()) {
        // Check WHERE clause if present
        if (query.where) {
            if (!evaluateWhereWithContext(varContext, query.where.get(), query)) {
                return; // Skip this combination if WHERE fails
            }
        }

        // Extract SELECT fields using variable context
        ResultRow row;
        for (const auto& field : query.select_fields) {
            std::string fieldName;
            std::string value;

            if (field.include_filename) {
                fieldName = "FILE_NAME";
                value = filename;
            } else {
                fieldName = field.components.back();

                // Resolve field using variable context
                value = resolveFieldWithContext(field, varContext, currentContext);
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
    for (const auto& node : iterationNodes) {
        // Bind this node to the variable
        varContext[forClause.variable] = node;

        // Recursively process next FOR clause
        processNestedForClauses(node, query, varContext, forClauseIndex + 1, filename, results);

        // Unbind variable (cleanup for next iteration)
        varContext.erase(forClause.variable);
    }
}

// Resolve field value using variable context
std::string QueryExecutor::resolveFieldWithContext(
    const FieldPath& field,
    const std::map<std::string, pugi::xml_node>& varContext,
    const pugi::xml_node& fallbackContext
) {
    std::string value;

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
    const WhereExpr* expr,
    const Query& query
) {
    if (!expr) return true;

    if (const auto* condition = dynamic_cast<const WhereCondition*>(expr)) {
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
        bool leftResult = evaluateWhereWithContext(varContext, logical->left.get(), query);
        bool rightResult = evaluateWhereWithContext(varContext, logical->right.get(), query);

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
        const std::string& orderField = query.order_by_fields[0];

        std::sort(allResults.begin(), allResults.end(),
            [&orderField](const ResultRow& a, const ResultRow& b) {
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
                    return aValue < bValue;
                }
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

} // namespace expocli
