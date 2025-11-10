#include "parser/lexer.h"
#include "parser/parser.h"
#include "executor/query_executor.h"
#include "utils/result_formatter.h"
#include <iostream>
#include <string>

void printUsage(const char* programName) {
    std::cout << "XML Query CLI - Phase 1 (MVP)\n";
    std::cout << "Usage: " << programName << " [query]\n\n";
    std::cout << "Query Syntax:\n";
    std::cout << "  SELECT <field>[,<field>...] FROM <path> [WHERE <condition>]\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " \"SELECT name FROM ./data WHERE calories < 500\"\n";
    std::cout << "  " << programName << " \"SELECT FILE_NAME,breakfast_menu/food/name FROM ./data\"\n\n";
    std::cout << "Field paths can use '.' or '/' as separators (e.g., food.name or food/name)\n";
    std::cout << "Special field: FILE_NAME returns the name of the XML file\n\n";
    std::cout << "Supported operators: =, !=, <, >, <=, >=\n";
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
