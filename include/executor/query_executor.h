#ifndef QUERY_EXECUTOR_H
#define QUERY_EXECUTOR_H

#include "parser/ast.h"
#include "executor/xml_navigator.h"
#include <vector>
#include <string>
#include <utility>

namespace expocli {

// Result row (multiple fields) - using vector to preserve field order
using ResultRow = std::vector<std::pair<std::string, std::string>>;

class QueryExecutor {
public:
    // Execute the query and return results
    static std::vector<ResultRow> execute(const Query& query);

private:
    // Get all XML files from directory
    static std::vector<std::string> getXmlFiles(const std::string& path);

    // Process a single XML file
    static std::vector<ResultRow> processFile(
        const std::string& filepath,
        const Query& query
    );
};

} // namespace expocli

#endif // QUERY_EXECUTOR_H
