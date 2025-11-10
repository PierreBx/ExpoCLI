#include "parser/parser.h"
#include <algorithm>

namespace xmlquery {

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens), current_(0) {}

std::unique_ptr<Query> Parser::parse() {
    auto query = std::make_unique<Query>();

    // Parse SELECT clause
    expect(TokenType::SELECT, "Expected SELECT keyword");

    // Parse field list
    query->select_fields.push_back(parseFieldPath());

    while (match(TokenType::COMMA)) {
        query->select_fields.push_back(parseFieldPath());
    }

    // Parse FROM clause
    expect(TokenType::FROM, "Expected FROM keyword");

    if (peek().type != TokenType::IDENTIFIER && peek().type != TokenType::STRING_LITERAL) {
        throw ParseError("Expected directory path after FROM");
    }

    query->from_path = advance().value;

    // Parse optional WHERE clause
    if (match(TokenType::WHERE)) {
        query->where = parseWhereClause();
    }

    // Ensure we're at the end
    if (!isAtEnd() && peek().type != TokenType::END_OF_INPUT) {
        throw ParseError("Unexpected tokens after query");
    }

    return query;
}

Token Parser::peek() const {
    if (current_ >= tokens_.size()) {
        return Token(TokenType::END_OF_INPUT, "", 0);
    }
    return tokens_[current_];
}

Token Parser::advance() {
    if (current_ < tokens_.size()) {
        return tokens_[current_++];
    }
    return Token(TokenType::END_OF_INPUT, "", 0);
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::isAtEnd() const {
    return current_ >= tokens_.size() || peek().type == TokenType::END_OF_INPUT;
}

void Parser::expect(TokenType type, const std::string& message) {
    if (!check(type)) {
        throw ParseError(message + " (got: " + peek().value + ")");
    }
    advance();
}

FieldPath Parser::parseFieldPath() {
    FieldPath field;

    // Handle FILE_NAME special case
    if (peek().value == "FILE_NAME") {
        field.include_filename = true;
        advance();
        return field;
    }

    // Parse first component
    if (peek().type != TokenType::IDENTIFIER) {
        throw ParseError("Expected field identifier");
    }

    field.components.push_back(advance().value);

    // Parse remaining components (separated by . or /)
    while (peek().type == TokenType::DOT || peek().type == TokenType::SLASH) {
        advance(); // consume separator

        if (peek().type != TokenType::IDENTIFIER) {
            throw ParseError("Expected identifier after separator");
        }

        field.components.push_back(advance().value);
    }

    return field;
}

std::unique_ptr<WhereCondition> Parser::parseWhereClause() {
    auto condition = std::make_unique<WhereCondition>();

    // Parse field path
    condition->field = parseFieldPath();

    // Parse comparison operator
    condition->op = parseComparisonOp();

    // Parse value
    Token valueToken = peek();
    if (valueToken.type == TokenType::NUMBER) {
        condition->value = advance().value;
        condition->is_numeric = true;
    }
    else if (valueToken.type == TokenType::STRING_LITERAL ||
             valueToken.type == TokenType::IDENTIFIER) {
        condition->value = advance().value;
        condition->is_numeric = false;
    }
    else {
        throw ParseError("Expected value in WHERE clause");
    }

    // Note: For MVP, we only support single WHERE condition
    // Future: Add AND/OR support

    return condition;
}

ComparisonOp Parser::parseComparisonOp() {
    Token op = peek();

    switch (op.type) {
        case TokenType::EQUALS:
            advance();
            return ComparisonOp::EQUALS;
        case TokenType::NOT_EQUALS:
            advance();
            return ComparisonOp::NOT_EQUALS;
        case TokenType::LESS_THAN:
            advance();
            return ComparisonOp::LESS_THAN;
        case TokenType::GREATER_THAN:
            advance();
            return ComparisonOp::GREATER_THAN;
        case TokenType::LESS_EQUAL:
            advance();
            return ComparisonOp::LESS_EQUAL;
        case TokenType::GREATER_EQUAL:
            advance();
            return ComparisonOp::GREATER_EQUAL;
        default:
            throw ParseError("Expected comparison operator");
    }
}

} // namespace xmlquery
