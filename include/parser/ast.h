#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

namespace expocli {

// Token types for the lexer
enum class TokenType {
    SELECT,
    DISTINCT,
    FROM,
    WHERE,
    ORDER,
    BY,
    LIMIT,
    OFFSET,
    ASC,
    DESC,
    COUNT,
    SUM,
    AVG,
    MIN,
    MAX,
    ASTERISK,
    SET,
    SHOW,
    XSD,
    DEST,
    GENERATE,
    XML,
    PREFIX,
    CHECK,
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
    IS,
    NOT,
    NULL_LITERAL,
    LIKE,
    REGEX_LITERAL,
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
    GREATER_EQUAL,
    IS_NULL,
    IS_NOT_NULL,
    LIKE,
    NOT_LIKE
};

// Aggregate function types
enum class AggregateFunc {
    NONE,
    COUNT,
    SUM,
    AVG,
    MIN,
    MAX
};

// AST Node for field selection (e.g., breakfast_menu.food.name)
struct FieldPath {
    std::vector<std::string> components; // ["breakfast_menu", "food", "name"]
    bool include_filename = false;       // Special case for FILE_NAME
    AggregateFunc aggregate = AggregateFunc::NONE; // Aggregate function if any
    bool is_count_star = false;          // Special case for COUNT(*)
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

// Sort direction for ORDER BY
enum class SortDirection {
    ASC,
    DESC
};

// ORDER BY field with direction
struct OrderByField {
    std::string field_name;
    SortDirection direction = SortDirection::ASC;
};

// Main Query AST
struct Query {
    std::vector<FieldPath> select_fields;     // Fields to select
    bool distinct = false;                     // DISTINCT flag
    std::string from_path;                     // Directory path
    std::unique_ptr<WhereExpr> where;          // Optional WHERE clause (can be condition or logical)
    std::vector<OrderByField> order_by_fields; // ORDER BY fields with direction
    int limit = -1;                            // LIMIT value (-1 means no limit)
    int offset = -1;                           // OFFSET value (-1 means no offset)
};

} // namespace expocli

#endif // AST_H
