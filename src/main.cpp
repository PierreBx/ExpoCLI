#include "parser/lexer.h"
#include "parser/parser.h"
#include "executor/query_executor.h"
#include "utils/result_formatter.h"
#include <iostream>
#include <string>

void printUsage(const char* programName) {
    std::cout << "XML Query CLI - Phase 2\n";
    std::cout << "Usage: " << programName << " [query]\n\n";
    std::cout << "Query Syntax:\n";
    std::cout << "  SELECT <field>[,<field>...] FROM <path>\n";
    std::cout << "  [WHERE <condition> [AND|OR <condition>...]]\n";
    std::cout << "  [ORDER BY <field>]\n";
    std::cout << "  [LIMIT <number>]\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " \"SELECT name FROM ./data WHERE calories < 500\"\n";
    std::cout << "  " << programName << " \"SELECT name FROM ./data WHERE year > 2000 AND price < 30\"\n";
    std::cout << "  " << programName << " \"SELECT name,price FROM ./data ORDER BY price LIMIT 5\"\n\n";
    std::cout << "Features:\n";
    std::cout << "  - Field paths can use '.' or '/' as separators (e.g., food.name or food/name)\n";
    std::cout << "  - Special field: FILE_NAME returns the name of the XML file\n";
    std::cout << "  - Comparison operators: =, !=, <, >, <=, >=\n";
    std::cout << "  - Logical operators: AND, OR (AND has higher precedence)\n";
    std::cout << "  - ORDER BY: Sort results by field (numeric or alphabetic)\n";
    std::cout << "  - LIMIT: Restrict number of results returned\n";
}

int main(int argc, char* argv[]) {
    try {
        // Check for arguments
        if (argc < 2) {
            printUsage(argv[0]);
            return 1;
        }

        // Handle help flag
        std::string arg = argv[1];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }

        // Get query from command line
        std::string query = argv[1];

        // Lexical analysis
        xmlquery::Lexer lexer(query);
        auto tokens = lexer.tokenize();

        // Syntax analysis
        xmlquery::Parser parser(tokens);
        auto ast = parser.parse();

        // Execute query
        auto results = xmlquery::QueryExecutor::execute(*ast);

        // Format and print results
        xmlquery::ResultFormatter::print(results);

        return 0;

    } catch (const xmlquery::ParseError& e) {
        std::cerr << "Parse Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
