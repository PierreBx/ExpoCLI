#ifndef RESULT_FORMATTER_H
#define RESULT_FORMATTER_H

#include "executor/query_executor.h"
#include <vector>
#include <string>
#include <ostream>
#include <iostream>

namespace expocli {

class ResultFormatter {
public:
    // Format and print results to output stream
    static void print(
        const std::vector<ResultRow>& results,
        std::ostream& out = std::cout
    );

    // Format results as plain text
    static std::string formatAsText(const std::vector<ResultRow>& results);
};

} // namespace expocli

#endif // RESULT_FORMATTER_H
