#ifndef QUERY_EXECUTOR_H
#define QUERY_EXECUTOR_H

#include "parser/ast.h"
#include "executor/xml_navigator.h"
#include <vector>
#include <string>
#include <utility>
#include <atomic>
#include <functional>
#include <map>

namespace expocli {

// Result row (multiple fields) - using vector to preserve field order
using ResultRow = std::vector<std::pair<std::string, std::string>>;

// Progress callback: (completed_files, total_files, thread_count)
using ProgressCallback = std::function<void(size_t, size_t, size_t)>;

// Execution statistics
struct ExecutionStats {
    size_t total_files = 0;
    size_t thread_count = 0;
    double execution_time_seconds = 0.0;
    bool used_threading = false;
};

class QueryExecutor {
public:
    // Execute the query and return results
    static std::vector<ResultRow> execute(const Query& query);

    // Execute with progress tracking (for VERBOSE mode)
    static std::vector<ResultRow> executeWithProgress(
        const Query& query,
        ProgressCallback progressCallback,
        ExecutionStats* stats = nullptr
    );

    // Validate query for ambiguous attributes (used in VERBOSE mode)
    // Returns a list of ambiguous attributes found
    static std::vector<std::string> checkForAmbiguousAttributes(const Query& query);

    // Get recommended thread count based on hardware
    static size_t getOptimalThreadCount();

    // Calculate if threading should be used based on file count and estimated work
    static bool shouldUseThreading(size_t fileCount);

private:
    // Get all XML files from directory
    static std::vector<std::string> getXmlFiles(const std::string& path);

    // Process a single XML file
    static std::vector<ResultRow> processFile(
        const std::string& filepath,
        const Query& query
    );

    // Process a single XML file with FOR clause context binding
    static std::vector<ResultRow> processFileWithForClauses(
        const std::string& filepath,
        const Query& query,
        const pugi::xml_document& doc,
        const std::string& filename
    );

    // Recursive function to process nested FOR clauses
    static void processNestedForClauses(
        const pugi::xml_node& currentContext,
        const Query& query,
        std::map<std::string, pugi::xml_node>& varContext,
        size_t forClauseIndex,
        const std::string& filename,
        std::vector<ResultRow>& results
    );

    // Resolve field value using variable context
    static std::string resolveFieldWithContext(
        const FieldPath& field,
        const std::map<std::string, pugi::xml_node>& varContext,
        const pugi::xml_node& fallbackContext
    );

    // Evaluate WHERE expression with variable context
    static bool evaluateWhereWithContext(
        const std::map<std::string, pugi::xml_node>& varContext,
        const WhereExpr* expr,
        const Query& query
    );

    // Execute query with multi-threading
    static std::vector<ResultRow> executeMultithreaded(
        const std::vector<std::string>& xmlFiles,
        const Query& query,
        size_t threadCount,
        std::atomic<size_t>* completedCounter = nullptr
    );
};

} // namespace expocli

#endif // QUERY_EXECUTOR_H
