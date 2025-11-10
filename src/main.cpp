#include "parser/lexer.h"
#include "parser/parser.h"
#include "executor/query_executor.h"
#include "utils/result_formatter.h"
#include <iostream>
#include <string>
#include <sstream>

void printWelcome() {
    std::cout << "XML Query CLI - Phase 2 (Interactive Mode)\n";
    std::cout << "Type 'help' for usage information, 'exit' or 'quit' to exit.\n";
    std::cout << "Enter SQL-like queries to search XML files.\n\n";
}

void printUsage(const char* programName) {
    std::cout << "expocli - a FT XML parser for FT/DSI/DIP\n";
    std::cout << "Usage:\n";
    std::cout << "  " << programName << "              # Start interactive mode\n";
    std::cout << "  " << programName << " [query]      # Execute single query\n\n";
    std::cout << "Query Syntax:\n";
    std::cout << "  SELECT <field>[,<field>...] FROM <path>\n";
    std::cout << "  [WHERE <condition> [AND|OR <condition>...]]\n";
    std::cout << "  [ORDER BY <field>]\n";
    std::cout << "  [LIMIT <number>]\n\n";
    std::cout << "Examples:\n";
    std::cout << "  SELECT name FROM ./data WHERE length < 500\n";
    std::cout << "  SELECT name FROM /path/to/files WHERE year > 2000 AND price < 30\n";
    std::cout << "  SELECT name,price FROM ../data ORDER BY price LIMIT 5\n\n";
    std::cout << "Features:\n";
    std::cout << "  - Field paths can use '.' or '/' as separators (e.g., food.name or food/name)\n";
    std::cout << "  - File paths can be quoted or unquoted (e.g., ./data or \"./data\")\n";
    std::cout << "  - Special field: FILE_NAME returns the name of the XML file\n";
    std::cout << "  - Comparison operators: =, !=, <, >, <=, >=\n";
    std::cout << "  - Logical operators: AND, OR (AND has higher precedence)\n";
    std::cout << "  - ORDER BY: Sort results by field (numeric or alphabetic)\n";
    std::cout << "  - LIMIT: Restrict number of results returned\n\n";
    std::cout << "Interactive Commands:\n";
    std::cout << "  help, \\h     Show this help message\n";
    std::cout << "  exit, quit   Exit the program\n";
    std::cout << "  \\c           Clear screen\n\n";
}

void executeQuery(const std::string& query) {
    if (query.empty()) {
        return;
    }

    try {
        // Lexical analysis
        expocli::Lexer lexer(query);
        auto tokens = lexer.tokenize();

        // Syntax analysis
        expocli::Parser parser(tokens);
        auto ast = parser.parse();

        // Execute query
        auto results = expocli::QueryExecutor::execute(*ast);

        // Format and print results
        expocli::ResultFormatter::print(results);

    } catch (const expocli::ParseError& e) {
        std::cerr << "Parse Error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void interactiveMode() {
    printWelcome();

    std::string line;
    std::string query;

    while (true) {
        // Show prompt
        if (query.empty()) {
            std::cout << "expocli> ";
        } else {
            std::cout << "      ... ";
        }
        std::cout.flush();

        // Read line
        if (!std::getline(std::cin, line)) {
            // EOF (Ctrl+D)
            std::cout << "\nBye!\n";
            break;
        }

        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\n\r"));
        line.erase(line.find_last_not_of(" \t\n\r") + 1);

        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        // Check for special commands
        if (line == "exit" || line == "quit" || line == "\\q") {
            std::cout << "Bye!\n";
            break;
        }

        if (line == "help" || line == "\\h" || line == "\\?") {
            printUsage("expocli");
            continue;
        }

        if (line == "\\c" || line == "clear") {
            std::cout << "\033[2J\033[1;1H"; // ANSI clear screen
            continue;
        }

        // Append to query
        if (!query.empty()) {
            query += " ";
        }
        query += line;

        // Check if query is complete (ends with semicolon or looks complete)
        // For simplicity, we'll execute on newline for now, unless line ends with backslash
        if (line.back() == '\\') {
            // Continue to next line
            query.pop_back(); // Remove backslash
            continue;
        }

        // Remove trailing semicolon if present
        if (!query.empty() && query.back() == ';') {
            query.pop_back();
        }

        // Execute the query
        executeQuery(query);
        std::cout << std::endl;

        // Reset for next query
        query.clear();
    }
}

int main(int argc, char* argv[]) {
    try {
        // No arguments: enter interactive mode
        if (argc < 2) {
            interactiveMode();
            return 0;
        }

        // Handle help flag
        std::string arg = argv[1];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }

        // Single query mode: execute query from command line
        std::string query = argv[1];
        executeQuery(query);

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
