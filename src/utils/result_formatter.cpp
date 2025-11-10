#include "utils/result_formatter.h"
#include <iostream>
#include <sstream>

namespace expocli {

void ResultFormatter::print(const std::vector<ResultRow>& results, std::ostream& out) {
    out << formatAsText(results);
}

std::string ResultFormatter::formatAsText(const std::vector<ResultRow>& results) {
    std::ostringstream oss;

    if (results.empty()) {
        oss << "No results found.\n";
        return oss.str();
    }

    // Print results
    for (const auto& row : results) {
        bool first = true;
        for (const auto& [field, value] : row) {
            if (!first) {
                oss << " | ";
            }
            oss << value;
            first = false;
        }
        oss << "\n";
    }

    if (results.size() == 1) {
        oss << "\n1 row returned.\n";
    } else {
        oss << "\n" << results.size() << " rows returned.\n";
    }

    return oss.str();
}

} // namespace expocli
