#include "parser/lexer.h"
#include "parser/parser.h"
#include "executor/query_executor.h"
#include "utils/result_formatter.h"
#include "utils/app_context.h"
#include "utils/command_handler.h"
#include <iostream>
#include <string>
#include <sstream>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

// History file path
static std::string historyFilePath;

// Signal handler for CTRL-C (SIGINT)
void signalHandler(int signal) {
    if (signal == SIGINT) {
        // Save history before exiting
        if (!historyFilePath.empty()) {
            write_history(historyFilePath.c_str());
        }
        std::cout << "\n\nBye!\n";
        std::exit(0);
    }
}

// Get user's home directory
std::string getHomeDirectory() {
    const char* home = getenv("HOME");
    if (home != nullptr) {
        return std::string(home);
    }

    // Fallback to passwd entry
    struct passwd* pw = getpwuid(getuid());
    if (pw != nullptr && pw->pw_dir != nullptr) {
        return std::string(pw->pw_dir);
    }

    return "";
}

// Get history file path
std::string getHistoryFilePath() {
    std::string home = getHomeDirectory();
    if (home.empty()) {
        return "";
    }
    return home + "/.expocli_history";
}

// Initialize command history
void initializeHistory() {
    historyFilePath = getHistoryFilePath();

    if (historyFilePath.empty()) {
        std::cerr << "Warning: Could not determine history file path\n";
        return;
    }

    // Set maximum history entries
    stifle_history(100);

    // Load existing history file
    read_history(historyFilePath.c_str());
}

// Save command history
void saveHistory() {
    if (!historyFilePath.empty()) {
        write_history(historyFilePath.c_str());
    }
}

void printWelcome() {
    std::cout << "XML Query CLI - Phase 2 (Interactive Mode)\n";
    std::cout << "Type 'help' for usage information.\n";
    std::cout << "Type 'exit', 'quit', or press Ctrl+C to exit.\n";
    std::cout << "Use UP/DOWN arrow keys to navigate command history.\n";
    std::cout << "Enter SQL-like queries to search XML files.\n";
    std::cout << "Note: Queries must be terminated with a semicolon (;)\n\n";
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
    std::cout << "  [LIMIT <number>];\n\n";
    std::cout << "Note: In interactive mode, queries MUST be terminated with a semicolon (;)\n";
    std::cout << "      Multi-line queries are supported - press Enter to continue.\n\n";
    std::cout << "Examples:\n";
    std::cout << "  SELECT name FROM ./data WHERE length < 500;\n";
    std::cout << "  SELECT name FROM /path/to/files WHERE year > 2000 AND price < 30;\n";
    std::cout << "  SELECT name,price FROM ../data ORDER BY price LIMIT 5;\n\n";
    std::cout << "Multi-line example:\n";
    std::cout << "  SELECT name, price\n";
    std::cout << "  FROM ./data\n";
    std::cout << "  WHERE (price < 5 OR calories > 900)\n";
    std::cout << "    AND price < 7;\n\n";
    std::cout << "Features:\n";
    std::cout << "  - Field paths can use '.' or '/' as separators (e.g., food.name or food/name)\n";
    std::cout << "  - File paths can be quoted or unquoted (e.g., ./data or \"./data\")\n";
    std::cout << "  - Special field: FILE_NAME returns the name of the XML file\n";
    std::cout << "  - Comparison operators: =, !=, <, >, <=, >=\n";
    std::cout << "  - Logical operators: AND, OR with parentheses support for precedence\n";
    std::cout << "  - Parentheses: Group conditions (e.g., (A OR B) AND C)\n";
    std::cout << "  - ORDER BY: Sort results by field (numeric or alphabetic)\n";
    std::cout << "  - LIMIT: Restrict number of results returned\n\n";
    std::cout << "Interactive Commands:\n";
    std::cout << "  help, \\h         Show this help message\n";
    std::cout << "  exit, quit       Exit the program\n";
    std::cout << "  Ctrl+C           Exit the program (SIGINT)\n";
    std::cout << "  \\c               Clear screen\n";
    std::cout << "  UP/DOWN arrows   Navigate command history (last 100 queries)\n\n";
    std::cout << "Configuration Commands:\n";
    std::cout << "  SET XSD <path>        Set XSD schema file path\n";
    std::cout << "  SET DEST <path>       Set destination directory path\n";
    std::cout << "  SHOW XSD              Display current XSD path\n";
    std::cout << "  SHOW DEST             Display current DEST path\n\n";
    std::cout << "Generation Commands:\n";
    std::cout << "  GENERATE XML <count>              Generate <count> XML files from XSD\n";
    std::cout << "  GENERATE XML <count> PREFIX <pre> Generate with custom filename prefix\n\n";
    std::cout << "Validation Commands:\n";
    std::cout << "  CHECK <file>        Validate a single XML file against XSD\n";
    std::cout << "  CHECK <directory>   Validate all XML files in a directory\n";
    std::cout << "  CHECK <pattern>     Validate files matching pattern (e.g., /path/*.xml)\n\n";
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
    // Register signal handler for CTRL-C
    std::signal(SIGINT, signalHandler);

    // Initialize command history
    initializeHistory();

    // Create application context and command handler
    expocli::AppContext context;
    expocli::CommandHandler commandHandler(context);

    printWelcome();

    std::string query;
    char* lineBuffer = nullptr;

    while (true) {
        // Set prompt based on whether we're continuing a query
        const char* prompt = query.empty() ? "expocli> " : "      -> ";

        // Read line with readline (supports arrow keys, history, etc.)
        lineBuffer = readline(prompt);

        // Check for EOF (Ctrl+D)
        if (lineBuffer == nullptr) {
            std::cout << "\nBye!\n";
            saveHistory();
            break;
        }

        // Convert to C++ string
        std::string line(lineBuffer);
        free(lineBuffer);

        // Trim leading whitespace only (preserve trailing for semicolon check)
        size_t firstNonSpace = line.find_first_not_of(" \t\n\r");
        if (firstNonSpace == std::string::npos) {
            // Line is all whitespace - treat as empty
            if (query.empty()) {
                continue;  // Skip completely empty lines when no query in progress
            } else {
                // Add newline to query and continue
                query += "\n";
                continue;
            }
        }

        // Trim leading whitespace
        line = line.substr(firstNonSpace);

        // Check for special commands (only when no query is in progress)
        if (query.empty()) {
            // Trim trailing whitespace for command comparison
            std::string trimmedLine = line;
            trimmedLine.erase(trimmedLine.find_last_not_of(" \t\n\r") + 1);

            if (trimmedLine == "exit" || trimmedLine == "quit" || trimmedLine == "\\q") {
                std::cout << "Bye!\n";
                saveHistory();
                break;
            }

            if (trimmedLine == "help" || trimmedLine == "\\h" || trimmedLine == "\\?") {
                printUsage("expocli");
                continue;
            }

            if (trimmedLine == "\\c" || trimmedLine == "clear") {
                std::cout << "\033[2J\033[1;1H"; // ANSI clear screen
                continue;
            }
        }

        // Look for semicolon in the line
        size_t semicolonPos = line.find(';');

        if (semicolonPos != std::string::npos) {
            // Found a semicolon - check if there's anything after it (except whitespace)
            std::string afterSemicolon = line.substr(semicolonPos + 1);
            size_t firstNonWhitespace = afterSemicolon.find_first_not_of(" \t\n\r");

            if (firstNonWhitespace != std::string::npos) {
                // There's non-whitespace content after the semicolon
                std::cerr << "Error: Unexpected text after semicolon: '"
                          << afterSemicolon.substr(firstNonWhitespace) << "'\n";
                query.clear();
                continue;
            }

            // Append everything before the semicolon to the query
            if (!query.empty()) {
                query += " ";
            }
            query += line.substr(0, semicolonPos);

            // Add completed query to history (with semicolon, only if not empty)
            if (!query.empty()) {
                std::string historyEntry = query + ";";
                add_history(historyEntry.c_str());
            }

            // Check if it's a SET or SHOW command
            if (!commandHandler.handleCommand(query)) {
                // Not a command, execute as a query
                executeQuery(query);
                std::cout << std::endl;
            }

            // Reset for next query
            query.clear();
        } else {
            // No semicolon found - continue accumulating the query
            if (!query.empty()) {
                query += " ";
            }
            query += line;
        }
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
