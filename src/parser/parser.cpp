#include "parser/parser.h"
#include <algorithm>

namespace expocli {

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens), current_(0) {}

std::unique_ptr<Query> Parser::parse() {
    auto query = std::make_unique<Query>();

    // Parse SELECT clause
    expect(TokenType::SELECT, "Expected SELECT keyword");

    // Parse field list (may include aggregations)
    query->select_fields.push_back(parseSelectField());

    while (match(TokenType::COMMA)) {
        query->select_fields.push_back(parseSelectField());
    }

    // Check if any fields are aggregations
    for (const auto& field : query->select_fields) {
        if (field.aggregate != AggregateFunc::NONE) {
            query->has_aggregates = true;
            break;
        }
    }

    // Parse FROM clause
    expect(TokenType::FROM, "Expected FROM keyword");

    query->from_path = parseFilePath();

    // Parse optional FOR clauses (can have multiple for nested iteration)
    while (check(TokenType::FOR)) {
        query->for_clauses.push_back(parseForClause());
    }

    // Parse optional WHERE clause
    if (match(TokenType::WHERE)) {
        query->where = parseWhereClause();
    }

    // Parse optional GROUP BY clause
    if (check(TokenType::GROUP)) {
        parseGroupByClause(*query);
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
        std::string tokenInfo = "token: " + peek().value + " (type: " + std::to_string(static_cast<int>(peek().type)) + ")";
        throw ParseError("Unexpected tokens after query - " + tokenInfo);
    }

    // Post-processing: Mark variable references in field paths
    if (!query->for_clauses.empty()) {
        // Check SELECT fields
        for (auto& field : query->select_fields) {
            if (!field.components.empty()) {
                if (query->isForVariable(field.components[0])) {
                    field.is_variable_ref = true;
                    field.variable_name = field.components[0];
                } else if (query->isPositionVariable(field.components[0])) {
                    // Position variables are single-component identifiers
                    field.is_variable_ref = true;
                    field.variable_name = field.components[0];
                    // Mark as special position variable (we'll handle differently)
                }
            }
        }

        // Check WHERE clause fields
        markVariableReferencesInWhere(query->where.get(), *query);
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

FieldPath Parser::parseSelectField() {
    FieldPath field;

    // Check if this is an aggregation function
    Token current = peek();

    if (current.type == TokenType::COUNT ||
        current.type == TokenType::SUM ||
        current.type == TokenType::AVG ||
        current.type == TokenType::MIN ||
        current.type == TokenType::MAX) {

        // Parse aggregation function
        AggregateFunc aggFunc;
        switch (current.type) {
            case TokenType::COUNT:
                aggFunc = AggregateFunc::COUNT;
                break;
            case TokenType::SUM:
                aggFunc = AggregateFunc::SUM;
                break;
            case TokenType::AVG:
                aggFunc = AggregateFunc::AVG;
                break;
            case TokenType::MIN:
                aggFunc = AggregateFunc::MIN;
                break;
            case TokenType::MAX:
                aggFunc = AggregateFunc::MAX;
                break;
            default:
                throw ParseError("Invalid aggregation function");
        }

        advance(); // consume function name
        expect(TokenType::LPAREN, "Expected '(' after aggregation function");

        // Parse argument (can be a variable name or field path like emp.salary)
        if (peek().type != TokenType::IDENTIFIER) {
            throw ParseError("Expected identifier in aggregation function");
        }

        std::string arg = advance().value;

        // Check for dot-separated field path (e.g., emp.salary)
        while (peek().type == TokenType::DOT) {
            advance(); // consume dot
            if (peek().type != TokenType::IDENTIFIER) {
                throw ParseError("Expected identifier after '.' in aggregation argument");
            }
            arg += "." + advance().value;
        }

        field.aggregate = aggFunc;
        field.aggregate_arg = arg;

        expect(TokenType::RPAREN, "Expected ')' after aggregation argument");

        // Optional AS alias
        if (match(TokenType::AS)) {
            if (peek().type != TokenType::IDENTIFIER) {
                throw ParseError("Expected alias name after AS");
            }
            field.alias = advance().value;
        }

        return field;
    }

    // Not an aggregation - parse as regular field
    field = parseFieldPath();

    // Check for optional AS alias
    if (match(TokenType::AS)) {
        if (peek().type != TokenType::IDENTIFIER) {
            throw ParseError("Expected alias name after AS");
        }
        field.alias = advance().value;
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
            current.type == TokenType::FOR ||
            current.type == TokenType::END_OF_INPUT) {
            break;
        }

        // Collect path components
        // Accept identifiers, slashes, dots, and also keywords that might appear in filenames
        // (like "xml", "data", etc.) but stop at statement keywords
        if (current.type == TokenType::IDENTIFIER ||
            current.type == TokenType::SLASH ||
            current.type == TokenType::DOT ||
            current.type == TokenType::XML ||
            current.type == TokenType::SET ||
            current.type == TokenType::SHOW ||
            current.type == TokenType::XSD ||
            current.type == TokenType::DEST ||
            current.type == TokenType::GENERATE ||
            current.type == TokenType::PREFIX ||
            current.type == TokenType::CHECK ||
            current.type == TokenType::VERBOSE ||
            current.type == TokenType::ASC ||
            current.type == TokenType::DESC ||
            current.type == TokenType::COUNT ||
            current.type == TokenType::SUM ||
            current.type == TokenType::AVG ||
            current.type == TokenType::MIN ||
            current.type == TokenType::MAX ||
            current.type == TokenType::AS ||
            current.type == TokenType::IN ||
            current.type == TokenType::AT ||
            current.type == TokenType::BY) {
            path += current.value;
            advance();
        } else {
            // Unknown token in path context or statement keyword
            break;
        }
    }

    if (path.empty()) {
        throw ParseError("Expected file or directory path after FROM");
    }

    return path;
}

// Parse FOR...IN clause (with optional AT position)
ForClause Parser::parseForClause() {
    ForClause forClause;

    // Expect FOR keyword
    expect(TokenType::FOR, "Expected FOR keyword");

    // Expect variable name (identifier)
    if (peek().type != TokenType::IDENTIFIER) {
        throw ParseError("Expected variable name after FOR");
    }
    forClause.variable = advance().value;

    // Expect IN keyword
    expect(TokenType::IN, "Expected IN keyword after variable name");

    // Parse field path
    forClause.path = parseFieldPath();

    // Optional AT keyword for position variable
    if (match(TokenType::AT)) {
        if (peek().type != TokenType::IDENTIFIER) {
            throw ParseError("Expected position variable name after AT");
        }
        forClause.position_var = advance().value;
        forClause.has_position = true;
    }

    return forClause;
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

void Parser::parseGroupByClause(Query& query) {
    expect(TokenType::GROUP, "Expected GROUP keyword");
    expect(TokenType::BY, "Expected BY keyword after GROUP");

    // Parse field to group by (can be dotted path like dept.name)
    if (peek().type != TokenType::IDENTIFIER) {
        throw ParseError("Expected field name after GROUP BY");
    }

    std::string fieldName = advance().value;

    // Check for dot-separated field path
    while (peek().type == TokenType::DOT) {
        advance(); // consume dot
        if (peek().type != TokenType::IDENTIFIER) {
            throw ParseError("Expected identifier after '.' in GROUP BY field");
        }
        fieldName += "." + advance().value;
    }

    query.group_by_fields.push_back(fieldName);

    // Support multiple GROUP BY fields separated by commas
    while (match(TokenType::COMMA)) {
        if (peek().type != TokenType::IDENTIFIER) {
            throw ParseError("Expected field name after comma in GROUP BY");
        }
        fieldName = advance().value;

        // Check for dot-separated field path
        while (peek().type == TokenType::DOT) {
            advance(); // consume dot
            if (peek().type != TokenType::IDENTIFIER) {
                throw ParseError("Expected identifier after '.' in GROUP BY field");
            }
            fieldName += "." + advance().value;
        }

        query.group_by_fields.push_back(fieldName);
    }
}

// Mark variable references in WHERE clause fields
void Parser::markVariableReferencesInWhere(WhereExpr* expr, const Query& query) {
    if (!expr) return;

    if (auto* condition = dynamic_cast<WhereCondition*>(expr)) {
        // Check if field starts with a variable name or position variable
        if (!condition->field.components.empty()) {
            if (query.isForVariable(condition->field.components[0])) {
                condition->field.is_variable_ref = true;
                condition->field.variable_name = condition->field.components[0];
            } else if (query.isPositionVariable(condition->field.components[0])) {
                condition->field.is_variable_ref = true;
                condition->field.variable_name = condition->field.components[0];
            }
        }
    } else if (auto* logical = dynamic_cast<WhereLogical*>(expr)) {
        // Recursively process left and right expressions
        markVariableReferencesInWhere(logical->left.get(), query);
        markVariableReferencesInWhere(logical->right.get(), query);
    }
}

} // namespace expocli
