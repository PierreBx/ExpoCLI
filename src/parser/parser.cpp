#include "parser/parser.h"
#include <algorithm>

namespace expocli {

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

    query->from_path = parseFilePath();

    // Parse optional WHERE clause
    if (match(TokenType::WHERE)) {
        query->where = parseWhereClause();
    }

    // Parse optional ORDER BY clause
    if (check(TokenType::ORDER)) {
        parseOrderByClause(*query);
    }

    // Parse optional LIMIT clause
    if (check(TokenType::LIMIT)) {
        parseLimitClause(*query);
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

std::string Parser::parseFilePath() {
    // If it's a string literal (quoted path), just return it
    if (peek().type == TokenType::STRING_LITERAL) {
        return advance().value;
    }

    // Otherwise, collect tokens to build the path (unquoted)
    // Path can contain: identifiers, dots, slashes
    // Stop when we hit: WHERE, ORDER, LIMIT, END_OF_INPUT
    std::string path;

    while (!isAtEnd()) {
        Token current = peek();

        // Stop if we hit a keyword
        if (current.type == TokenType::WHERE ||
            current.type == TokenType::ORDER ||
            current.type == TokenType::LIMIT ||
            current.type == TokenType::END_OF_INPUT) {
            break;
        }

        // Collect path components
        if (current.type == TokenType::IDENTIFIER ||
            current.type == TokenType::SLASH ||
            current.type == TokenType::DOT) {
            path += current.value;
            advance();
        } else {
            // Unknown token in path context
            break;
        }
    }

    if (path.empty()) {
        throw ParseError("Expected file or directory path after FROM");
    }

    return path;
}

// Parse WHERE clause (entry point)
std::unique_ptr<WhereExpr> Parser::parseWhereClause() {
    return parseWhereExpression();
}

// Parse OR-level expression (lowest precedence)
std::unique_ptr<WhereExpr> Parser::parseWhereExpression() {
    return parseWhereOr();
}

std::unique_ptr<WhereExpr> Parser::parseWhereOr() {
    auto left = parseWhereAnd();

    while (match(TokenType::OR)) {
        auto logical = std::make_unique<WhereLogical>();
        logical->op = LogicalOp::OR;
        logical->left = std::move(left);
        logical->right = parseWhereAnd();
        left = std::move(logical);
    }

    return left;
}

std::unique_ptr<WhereExpr> Parser::parseWhereAnd() {
    auto left = parseWherePrimary();

    while (match(TokenType::AND)) {
        auto logical = std::make_unique<WhereLogical>();
        logical->op = LogicalOp::AND;
        logical->left = std::move(left);
        logical->right = parseWherePrimary();
        left = std::move(logical);
    }

    return left;
}

std::unique_ptr<WhereExpr> Parser::parseWherePrimary() {
    // Handle parenthesized expressions
    if (match(TokenType::LPAREN)) {
        auto expr = parseWhereExpression();
        expect(TokenType::RPAREN, "Expected closing parenthesis");
        return expr;
    }

    // Otherwise, parse a simple condition
    return parseWhereCondition();
}

std::unique_ptr<WhereExpr> Parser::parseWhereCondition() {
    auto condition = std::make_unique<WhereCondition>();

    // Parse field path
    condition->field = parseFieldPath();

    // Check for IS NULL, IS NOT NULL, LIKE, or IS NOT LIKE
    if (peek().type == TokenType::IS) {
        advance(); // consume IS

        // Check for NOT
        if (peek().type == TokenType::NOT) {
            advance(); // consume NOT

            // Must be followed by NULL or LIKE
            if (peek().type == TokenType::NULL_LITERAL) {
                advance(); // consume NULL
                condition->op = ComparisonOp::IS_NOT_NULL;
                condition->value = "";
                condition->is_numeric = false;
                return condition;
            }
            else if (peek().type == TokenType::LIKE) {
                advance(); // consume LIKE
                // Expect /regex/
                expect(TokenType::SLASH, "Expected '/' to start regex pattern");
                condition->op = ComparisonOp::NOT_LIKE;
                condition->value = parseRegexPattern();
                condition->is_numeric = false;
                return condition;
            }
            else {
                throw ParseError("Expected NULL or LIKE after IS NOT");
            }
        }
        else if (peek().type == TokenType::NULL_LITERAL) {
            advance(); // consume NULL
            condition->op = ComparisonOp::IS_NULL;
            condition->value = "";
            condition->is_numeric = false;
            return condition;
        }
        else {
            throw ParseError("Expected NULL or NOT after IS");
        }
    }
    else if (peek().type == TokenType::LIKE) {
        advance(); // consume LIKE
        // Expect /regex/
        expect(TokenType::SLASH, "Expected '/' to start regex pattern");
        condition->op = ComparisonOp::LIKE;
        condition->value = parseRegexPattern();
        condition->is_numeric = false;
        return condition;
    }

    // Parse standard comparison operator
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

std::string Parser::parseRegexPattern() {
    // We've already consumed the opening '/'
    // Now collect all tokens until we hit the closing '/'
    std::string pattern;

    while (!isAtEnd() && peek().type != TokenType::SLASH) {
        Token token = advance();

        // Add the token value to the pattern
        if (!pattern.empty() && token.type != TokenType::DOT &&
            token.type != TokenType::COMMA && token.type != TokenType::LPAREN &&
            token.type != TokenType::RPAREN) {
            // Add space before this token if it's not punctuation
            if (pattern.back() != '.' && pattern.back() != '(' &&
                pattern.back() != ')' && pattern.back() != '*' &&
                pattern.back() != '+' && pattern.back() != '?' &&
                pattern.back() != '|' && pattern.back() != '[' &&
                pattern.back() != ']') {
                // Don't add space, build directly
            }
        }

        // Append the token value directly without spaces
        pattern += token.value;
    }

    // Expect closing '/'
    expect(TokenType::SLASH, "Expected '/' to close regex pattern");

    return pattern;
}

void Parser::parseOrderByClause(Query& query) {
    expect(TokenType::ORDER, "Expected ORDER keyword");
    expect(TokenType::BY, "Expected BY keyword after ORDER");

    // Parse field to order by
    if (peek().type != TokenType::IDENTIFIER) {
        throw ParseError("Expected field name after ORDER BY");
    }

    std::string fieldName = advance().value;
    query.order_by_fields.push_back(fieldName);

    // Skip optional ASC/DESC for now (default is ASC)
    if (match(TokenType::ASC) || match(TokenType::DESC)) {
        // For Phase 2, we'll just store the field name
        // Future enhancement: store sort direction
    }

    // Support multiple ORDER BY fields separated by commas
    while (match(TokenType::COMMA)) {
        if (peek().type != TokenType::IDENTIFIER) {
            throw ParseError("Expected field name after comma in ORDER BY");
        }
        fieldName = advance().value;
        query.order_by_fields.push_back(fieldName);

        // Skip optional ASC/DESC
        if (match(TokenType::ASC) || match(TokenType::DESC)) {
            // Future enhancement: store sort direction
        }
    }
}

void Parser::parseLimitClause(Query& query) {
    expect(TokenType::LIMIT, "Expected LIMIT keyword");

    if (peek().type != TokenType::NUMBER) {
        throw ParseError("Expected number after LIMIT");
    }

    try {
        query.limit = std::stoi(advance().value);
        if (query.limit < 0) {
            throw ParseError("LIMIT value must be non-negative");
        }
    } catch (const std::invalid_argument&) {
        throw ParseError("Invalid LIMIT value");
    } catch (const std::out_of_range&) {
        throw ParseError("LIMIT value out of range");
    }
}

} // namespace expocli
