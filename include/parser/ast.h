#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

namespace xmlquery {

// Token types for the lexer
enum class TokenType {
    SELECT,
    FROM,
    WHERE,
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

// AST Node for WHERE condition
struct WhereCondition {
    FieldPath field;
    ComparisonOp op;
    std::string value;
    bool is_numeric;
};

// Main Query AST
struct Query {
    std::vector<FieldPath> select_fields;  // Fields to select
    std::string from_path;                  // Directory path
    std::unique_ptr<WhereCondition> where; // Optional WHERE clause
};

} // namespace xmlquery

#endif // AST_H
