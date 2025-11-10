#include "utils/command_handler.h"
#include "parser/lexer.h"
#include "generator/xsd_parser.h"
#include "generator/xml_generator.h"
#include <iostream>
#include <filesystem>
#include <fstream>

namespace expocli {

CommandHandler::CommandHandler(AppContext& context)
    : context_(context) {}

bool CommandHandler::handleCommand(const std::string& input) {
    // Tokenize the input to identify the command
    Lexer lexer(input);
    auto tokens = lexer.tokenize();

    if (tokens.empty() || tokens[0].type == TokenType::END_OF_INPUT) {
        return false;
    }

    // Check if it's a SET command
    if (tokens[0].type == TokenType::SET) {
        return handleSetCommand(input);
    }

    // Check if it's a SHOW command
    if (tokens[0].type == TokenType::SHOW) {
        return handleShowCommand(input);
    }

    // Check if it's a GENERATE command
    if (tokens[0].type == TokenType::GENERATE) {
        return handleGenerateCommand(input);
    }

    // Not a recognized command, treat as query
    return false;
}

bool CommandHandler::handleSetCommand(const std::string& input) {
    Lexer lexer(input);
    auto tokens = lexer.tokenize();

    // Expect: SET <XSD|DEST> <path>
    if (tokens.size() < 3) {
        std::cerr << "Error: SET command requires a parameter (XSD or DEST) and a path\n";
        std::cerr << "Usage: SET XSD /path/to/file.xsd\n";
        std::cerr << "       SET DEST /path/to/directory\n";
        return true;
    }

    TokenType paramType = tokens[1].type;

    // Collect path from remaining tokens
    std::string path;
    for (size_t i = 2; i < tokens.size(); ++i) {
        if (tokens[i].type == TokenType::END_OF_INPUT) {
            break;
        }
        // Don't add spaces for path separators or punctuation
        if (!path.empty() &&
            tokens[i].type != TokenType::DOT &&
            tokens[i].type != TokenType::SLASH &&
            tokens[i].type != TokenType::COMMA &&
            tokens[i-1].type != TokenType::DOT &&
            tokens[i-1].type != TokenType::SLASH) {
            // For paths, only add space if previous token wasn't a separator
            // and we're dealing with identifiers (like quoted strings with spaces)
            if (tokens[i].type == TokenType::STRING_LITERAL ||
                (tokens[i-1].type == TokenType::STRING_LITERAL)) {
                path += " ";
            }
        }
        path += tokens[i].value;
    }

    if (path.empty()) {
        std::cerr << "Error: Path cannot be empty\n";
        return true;
    }

    // Handle based on parameter type
    if (paramType == TokenType::XSD) {
        setXsdPath(path);
    } else if (paramType == TokenType::DEST) {
        setDestPath(path);
    } else {
        std::cerr << "Error: Unknown SET parameter. Use XSD or DEST\n";
    }

    return true;
}

bool CommandHandler::handleShowCommand(const std::string& input) {
    Lexer lexer(input);
    auto tokens = lexer.tokenize();

    // Expect: SHOW <XSD|DEST>
    if (tokens.size() < 2) {
        std::cerr << "Error: SHOW command requires a parameter (XSD or DEST)\n";
        std::cerr << "Usage: SHOW XSD\n";
        std::cerr << "       SHOW DEST\n";
        return true;
    }

    TokenType paramType = tokens[1].type;

    // Handle based on parameter type
    if (paramType == TokenType::XSD) {
        showXsdPath();
    } else if (paramType == TokenType::DEST) {
        showDestPath();
    } else {
        std::cerr << "Error: Unknown SHOW parameter. Use XSD or DEST\n";
    }

    return true;
}

void CommandHandler::setXsdPath(const std::string& path) {
    if (validateXsdFile(path)) {
        context_.setXsdPath(path);
        std::cout << "XSD path set to: " << path << "\n";
    }
}

void CommandHandler::setDestPath(const std::string& path) {
    if (validateAndCreateDestDirectory(path)) {
        context_.setDestPath(path);
        std::cout << "DEST path set to: " << path << "\n";
    }
}

void CommandHandler::showXsdPath() {
    if (context_.hasXsdPath()) {
        std::cout << "XSD: " << context_.getXsdPath().value() << "\n";
    } else {
        std::cout << "XSD: (not set)\n";
    }
}

void CommandHandler::showDestPath() {
    if (context_.hasDestPath()) {
        std::cout << "DEST: " << context_.getDestPath().value() << "\n";
    } else {
        std::cout << "DEST: (not set)\n";
    }
}

bool CommandHandler::validateXsdFile(const std::string& path) {
    // Check if file exists
    if (!std::filesystem::exists(path)) {
        std::cerr << "Error: XSD file does not exist: " << path << "\n";
        return false;
    }

    // Check if it's a regular file
    if (!std::filesystem::is_regular_file(path)) {
        std::cerr << "Error: Path is not a file: " << path << "\n";
        return false;
    }

    // Check if file has .xsd extension
    std::filesystem::path filePath(path);
    if (filePath.extension() != ".xsd") {
        std::cerr << "Warning: File does not have .xsd extension: " << path << "\n";
    }

    // Basic validation: check if file can be opened and is not empty
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open XSD file: " << path << "\n";
        return false;
    }

    // Check if file is not empty
    file.seekg(0, std::ios::end);
    if (file.tellg() == 0) {
        std::cerr << "Error: XSD file is empty: " << path << "\n";
        return false;
    }

    // Basic XML structure check
    file.seekg(0, std::ios::beg);
    std::string firstLine;
    std::getline(file, firstLine);

    // Very basic check: should start with XML declaration or have schema tag
    if (firstLine.find("<?xml") == std::string::npos &&
        firstLine.find("<xs:schema") == std::string::npos &&
        firstLine.find("<xsd:schema") == std::string::npos &&
        firstLine.find("<schema") == std::string::npos) {
        std::cerr << "Warning: File may not be a valid XSD (no XML/schema declaration found)\n";
    }

    return true;
}

bool CommandHandler::validateAndCreateDestDirectory(const std::string& path) {
    // Check if directory exists
    if (std::filesystem::exists(path)) {
        if (!std::filesystem::is_directory(path)) {
            std::cerr << "Error: Path exists but is not a directory: " << path << "\n";
            return false;
        }
        // Directory exists and is valid
        return true;
    }

    // Directory doesn't exist, ask user if they want to create it
    std::cout << "Directory does not exist: " << path << "\n";
    std::cout << "Do you want to create it? (y/n): ";

    std::string response;
    std::getline(std::cin, response);

    if (response.empty() || (response[0] != 'y' && response[0] != 'Y')) {
        std::cout << "Directory creation cancelled.\n";
        return false;
    }

    // Try to create the directory
    try {
        std::filesystem::create_directories(path);
        std::cout << "Directory created successfully: " << path << "\n";
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: Failed to create directory: " << e.what() << "\n";
        return false;
    }
}

bool CommandHandler::handleGenerateCommand(const std::string& input) {
    Lexer lexer(input);
    auto tokens = lexer.tokenize();

    // Expect: GENERATE XML <count> [PREFIX <prefix>]
    if (tokens.size() < 3) {
        std::cerr << "Error: GENERATE command requires XML and count\n";
        std::cerr << "Usage: GENERATE XML <count>\n";
        std::cerr << "       GENERATE XML <count> PREFIX <prefix>\n";
        return true;
    }

    // Check for XML keyword
    if (tokens[1].type != TokenType::XML) {
        std::cerr << "Error: Expected XML after GENERATE\n";
        return true;
    }

    // Get count
    if (tokens[2].type != TokenType::NUMBER) {
        std::cerr << "Error: Expected number after GENERATE XML\n";
        return true;
    }

    int count = 0;
    try {
        count = std::stoi(tokens[2].value);
    } catch (...) {
        std::cerr << "Error: Invalid count value\n";
        return true;
    }

    if (count <= 0) {
        std::cerr << "Error: Count must be positive\n";
        return true;
    }

    // Check for optional PREFIX
    std::string prefix = "generated_";
    if (tokens.size() >= 5 && tokens[3].type == TokenType::PREFIX) {
        if (tokens[4].type == TokenType::IDENTIFIER ||
            tokens[4].type == TokenType::STRING_LITERAL) {
            prefix = tokens[4].value;
        }
    }

    // Check if XSD is set
    if (!context_.hasXsdPath()) {
        std::cerr << "Error: XSD path not set. Use SET XSD <path> first\n";
        return true;
    }

    // Check if DEST is set
    if (!context_.hasDestPath()) {
        std::cerr << "Error: DEST path not set. Use SET DEST <path> first\n";
        return true;
    }

    std::string xsdPath = context_.getXsdPath().value();
    std::string destPath = context_.getDestPath().value();

    try {
        // Parse XSD schema
        std::cout << "Parsing XSD schema: " << xsdPath << "\n";
        auto schema = XsdParser::parse(xsdPath);

        // Generate XML files
        XmlGenerator generator;
        generator.generateFiles(*schema, count, destPath, prefix);

    } catch (const std::exception& e) {
        std::cerr << "Error generating XML files: " << e.what() << "\n";
        return true;
    }

    return true;
}

} // namespace expocli
