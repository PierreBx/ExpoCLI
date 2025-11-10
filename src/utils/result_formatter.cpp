#include "utils/result_formatter.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace expocli {

void ResultFormatter::print(const std::vector<ResultRow>& results, std::ostream& out) {
    out << formatAsText(results);
}

std::string ResultFormatter::formatAsText(const std::vector<ResultRow>& results) {
    std::ostringstream oss;

    // Add blank line before results
    oss << "\n";

    if (results.empty()) {
        oss << "No results found.\n";
        return oss.str();
    }

    const int MAX_COLUMN_WIDTH = 50;
    const std::string TRUNCATE_INDICATOR = " 🔴"; // Red circle emoji

    // Extract column headers from first row
    std::vector<std::string> headers;
    for (const auto& [field, value] : results[0]) {
        headers.push_back(field);
    }

    // Calculate column widths based on headers and data
    std::vector<size_t> columnWidths(headers.size(), 0);

    // Initialize with header widths
    for (size_t i = 0; i < headers.size(); ++i) {
        columnWidths[i] = headers[i].length();
    }

    // Update with data widths (considering truncation)
    for (const auto& row : results) {
        size_t colIdx = 0;
        for (const auto& [field, value] : row) {
            size_t displayWidth = value.length();
            if (displayWidth > MAX_COLUMN_WIDTH) {
                // Width will be MAX_COLUMN_WIDTH - 1 (for truncation) + indicator length
                displayWidth = MAX_COLUMN_WIDTH - 1 + TRUNCATE_INDICATOR.length();
            }
            columnWidths[colIdx] = std::max(columnWidths[colIdx], displayWidth);
            colIdx++;
        }
    }

    // Print header row
    for (size_t i = 0; i < headers.size(); ++i) {
        if (i > 0) {
            oss << " | ";
        }
        oss << std::left << std::setw(columnWidths[i]) << headers[i];
    }
    oss << "\n";

    // Print separator line
    for (size_t i = 0; i < headers.size(); ++i) {
        if (i > 0) {
            oss << "-+-";
        }
        oss << std::string(columnWidths[i], '-');
    }
    oss << "\n";

    // Print data rows
    for (const auto& row : results) {
        size_t colIdx = 0;
        for (const auto& [field, value] : row) {
            if (colIdx > 0) {
                oss << " | ";
            }

            // Handle truncation
            std::string displayValue = value;
            if (value.length() > MAX_COLUMN_WIDTH) {
                displayValue = value.substr(0, MAX_COLUMN_WIDTH - 1) + TRUNCATE_INDICATOR;
            }

            oss << std::left << std::setw(columnWidths[colIdx]) << displayValue;
            colIdx++;
        }
        oss << "\n";
    }

    // Print row count
    oss << "\n";
    if (results.size() == 1) {
        oss << "1 row returned.\n";
    } else {
        oss << results.size() << " rows returned.\n";
    }

    return oss.str();
}

} // namespace expocli
