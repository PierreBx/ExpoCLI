#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

namespace expocli {

// Token types for the lexer
enum class TokenType {
    SELECT,
    FROM,
    WHERE,
    ORDER,
    BY,
    LIMIT,
    ASC,
    DESC,
    IDENTIFIER,
    STRING_LITERAL,
    NUMBER,
    DOT,
    SLASH,
    COMMA,
    LESS_THAN,
    GREATER_THAN,
    EQUALS,
    NOT_EQUALS,
    LESS_EQUAL,
    GREATER_EQUAL,
    AND,
    OR,
    LPAREN,
    RPAREN,
    END_OF_INPUT,
    INVALID
};

// Token structure
struct Token {
    TokenType type;
    std::string value;
    size_t position;

    Token(TokenType t = TokenType::INVALID, const std::string& v = "", size_t pos = 0)
        : type(t), value(v), position(pos) {}
};

// Comparison operators for WHERE clause
enum class ComparisonOp {
    EQUALS,
    NOT_EQUALS,
    LESS_THAN,
    GREATER_THAN,
    LESS_EQUAL,
    GREATER_EQUAL
};

// AST Node for field selection (e.g., breakfast_menu.food.name)
struct FieldPath {
    std::vector<std::string> components; // ["breakfast_menu", "food", "name"]
    bool include_filename = false;       // Special case for FILE_NAME
};

// Logical operators for combining conditions
enum class LogicalOp {
    NONE,  // Single condition
    AND,
    OR
};

// Base class for WHERE expressions
struct WhereExpr {
    virtual ~WhereExpr() = default;
};

// Simple comparison condition
struct WhereCondition : public WhereExpr {
    FieldPath field;
    ComparisonOp op;
    std::string value;
    bool is_numeric;
};

// Logical combination of conditions (AND/OR)
struct WhereLogical : public WhereExpr {
    LogicalOp op;
    std::unique_ptr<WhereExpr> left;
    std::unique_ptr<WhereExpr> right;
};

// Main Query AST
struct Query {
    std::vector<FieldPath> select_fields;     // Fields to select
    std::string from_path;                     // Directory path
    std::unique_ptr<WhereExpr> where;          // Optional WHERE clause (can be condition or logical)
    std::vector<std::string> order_by_fields;  // ORDER BY fields (Phase 2)
    int limit = -1;                            // LIMIT value (Phase 2, -1 means no limit)
};

} // namespace expocli

#endif // AST_H
