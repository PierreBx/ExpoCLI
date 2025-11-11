#include "parser/lexer.h"
#include <cctype>
#include <algorithm>

namespace expocli {

Lexer::Lexer(const std::string& input)
    : input_(input), position_(0) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;

        char current = peek();

        // Single-character tokens
        if (current == '.') {
            tokens.emplace_back(TokenType::DOT, ".", position_);
            advance();
        }
        else if (current == '/') {
            tokens.emplace_back(TokenType::SLASH, "/", position_);
            advance();
        }
        else if (current == ',') {
            tokens.emplace_back(TokenType::COMMA, ",", position_);
            advance();
        }
        else if (current == '(') {
            tokens.emplace_back(TokenType::LPAREN, "(", position_);
            advance();
        }
        else if (current == ')') {
            tokens.emplace_back(TokenType::RPAREN, ")", position_);
            advance();
        }
        // Comparison operators
        else if (current == '<') {
            advance();
            if (!isAtEnd() && peek() == '=') {
                tokens.emplace_back(TokenType::LESS_EQUAL, "<=", position_ - 1);
                advance();
            } else {
                tokens.emplace_back(TokenType::LESS_THAN, "<", position_ - 1);
            }
        }
        else if (current == '>') {
            advance();
            if (!isAtEnd() && peek() == '=') {
                tokens.emplace_back(TokenType::GREATER_EQUAL, ">=", position_ - 1);
                advance();
            } else {
                tokens.emplace_back(TokenType::GREATER_THAN, ">", position_ - 1);
            }
        }
        else if (current == '=') {
            tokens.emplace_back(TokenType::EQUALS, "=", position_);
            advance();
        }
        else if (current == '!') {
            advance();
            if (!isAtEnd() && peek() == '=') {
                tokens.emplace_back(TokenType::NOT_EQUALS, "!=", position_ - 1);
                advance();
            } else {
                tokens.emplace_back(TokenType::INVALID, "!", position_ - 1);
            }
        }
        // String literals
        else if (current == '"' || current == '\'') {
            tokens.push_back(readString());
        }
        // Numbers
        else if (std::isdigit(current)) {
            tokens.push_back(readNumber());
        }
        // Identifiers and keywords
        else if (std::isalpha(current) || current == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        }
        else {
            // Unknown character
            tokens.emplace_back(TokenType::INVALID, std::string(1, current), position_);
            advance();
        }
    }

    tokens.emplace_back(TokenType::END_OF_INPUT, "", position_);
    return tokens;
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return input_[position_];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    return input_[position_++];
}

void Lexer::skipWhitespace() {
    while (!isAtEnd() && std::isspace(peek())) {
        advance();
    }
}

bool Lexer::isAtEnd() const {
    return position_ >= input_.length();
}

Token Lexer::readIdentifierOrKeyword() {
    size_t start = position_;
    std::string value;

    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
        value += advance();
    }

    TokenType type = identifyKeyword(value);
    return Token(type, value, start);
}

Token Lexer::readNumber() {
    size_t start = position_;
    std::string value;

    while (!isAtEnd() && (std::isdigit(peek()) || peek() == '.')) {
        value += advance();
    }

    return Token(TokenType::NUMBER, value, start);
}

Token Lexer::readString() {
    char quote = advance(); // consume opening quote
    size_t start = position_ - 1;
    std::string value;

    while (!isAtEnd() && peek() != quote) {
        value += advance();
    }

    if (isAtEnd()) {
        return Token(TokenType::INVALID, value, start);
    }

    advance(); // consume closing quote
    return Token(TokenType::STRING_LITERAL, value, start);
}

TokenType Lexer::identifyKeyword(const std::string& word) const {
    std::string upper = word;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    if (upper == "SELECT") return TokenType::SELECT;
    if (upper == "FROM") return TokenType::FROM;
    if (upper == "WHERE") return TokenType::WHERE;
    if (upper == "AND") return TokenType::AND;
    if (upper == "OR") return TokenType::OR;
    if (upper == "ORDER") return TokenType::ORDER;
    if (upper == "BY") return TokenType::BY;
    if (upper == "LIMIT") return TokenType::LIMIT;
    if (upper == "ASC") return TokenType::ASC;
    if (upper == "DESC") return TokenType::DESC;
    if (upper == "IS") return TokenType::IS;
    if (upper == "NOT") return TokenType::NOT;
    if (upper == "NULL") return TokenType::NULL_LITERAL;
    if (upper == "LIKE") return TokenType::LIKE;
    if (upper == "SET") return TokenType::SET;
    if (upper == "SHOW") return TokenType::SHOW;
    if (upper == "XSD") return TokenType::XSD;
    if (upper == "DEST") return TokenType::DEST;
    if (upper == "GENERATE") return TokenType::GENERATE;
    if (upper == "XML") return TokenType::XML;
    if (upper == "PREFIX") return TokenType::PREFIX;
    if (upper == "CHECK") return TokenType::CHECK;

    return TokenType::IDENTIFIER;
}

} // namespace expocli
