#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include <vector>
#include <memory>
#include <stdexcept>

namespace expocli {

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& message)
        : std::runtime_error(message) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    // Parse the tokens into a Query AST
    std::unique_ptr<Query> parse();

private:
    std::vector<Token> tokens_;
    size_t current_;

    // Helper methods
    Token peek() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool isAtEnd() const;
    void expect(TokenType type, const std::string& message);

    // Parsing methods
    FieldPath parseFieldPath();
    std::string parseFilePath();  // Parse filesystem path (quoted or unquoted)
    ForClause parseForClause();   // Parse FOR...IN clause
    std::unique_ptr<WhereExpr> parseWhereClause();
    std::unique_ptr<WhereExpr> parseWhereExpression();
    std::unique_ptr<WhereExpr> parseWhereOr();
    std::unique_ptr<WhereExpr> parseWhereAnd();
    std::unique_ptr<WhereExpr> parseWherePrimary();
    std::unique_ptr<WhereExpr> parseWhereCondition();
    ComparisonOp parseComparisonOp();
    std::string parseRegexPattern();  // Parse regex pattern between / delimiters
    void parseOrderByClause(Query& query);
    void parseLimitClause(Query& query);
};

} // namespace expocli

#endif // PARSER_H
