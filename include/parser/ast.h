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
    AT,
    SET,
    SHOW,
    XSD,
    DEST,
    GENERATE,
    XML,
    PREFIX,
    CHECK,
    VERBOSE,
    FOR,
    IN,
    GROUP,
    HAVING,
    AS,
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
    NOT_LIKE,
    IN,
    NOT_IN
};

// Aggregate function types
enum class AggregateFunc {
    NONE,   // Not an aggregation
    COUNT,
    SUM,
    AVG,
    MIN,
    MAX
};

// AST Node for field selection (e.g., breakfast_menu.food.name or @isbn)
struct FieldPath {
    std::vector<std::string> components; // ["breakfast_menu", "food", "name"]
    bool include_filename = false;       // Special case for FILE_NAME
    bool is_variable_ref = false;        // True if first component is a FOR variable
    std::string variable_name = "";      // Variable name if is_variable_ref is true

    // Aggregation support
    AggregateFunc aggregate = AggregateFunc::NONE;  // Aggregation function (if any)
    std::string aggregate_arg = "";                  // Argument to aggregation (e.g., variable name for COUNT(emp))
    std::string alias = "";                          // AS alias for the field
    bool is_count_star = false;                      // Special case for COUNT(*)

    // XML attribute support
    bool is_attribute = false;           // True if this is an XML attribute (@attr)
    std::string attribute_name;          // Attribute name (when is_attribute = true)
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
    std::vector<std::string> values;  // For IN/NOT_IN operators
};

// Logical combination of conditions (AND/OR)
struct WhereLogical : public WhereExpr {
    LogicalOp op;
    std::unique_ptr<WhereExpr> left;
    std::unique_ptr<WhereExpr> right;
};

// FOR clause for context binding
// Example: FOR emp IN employee AT emp_idx
struct ForClause {
    std::string variable;      // Variable name (e.g., "emp")
    FieldPath path;            // Path to iterate over (e.g., "employee" or "department.employee")
    std::string position_var;  // Position variable name (e.g., "emp_idx"), empty if no AT
    bool has_position = false; // True if AT clause is present
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
    std::vector<ForClause> for_clauses;        // Optional FOR clauses for iteration context
    std::unique_ptr<WhereExpr> where;          // Optional WHERE clause (can be condition or logical)
    std::vector<std::string> group_by_fields;  // GROUP BY fields
    std::unique_ptr<WhereExpr> having;         // Optional HAVING clause (filters aggregated results)
    std::vector<OrderByField> order_by_fields; // ORDER BY fields with direction
    int limit = -1;                            // LIMIT value (-1 means no limit)
    int offset = -1;                           // OFFSET value (-1 means no offset)
    bool has_aggregates = false;               // True if SELECT contains aggregation functions

    // Helper: Check if identifier is a FOR variable
    bool isForVariable(const std::string& name) const {
        for (const auto& forClause : for_clauses) {
            if (forClause.variable == name) {
                return true;
            }
        }
        return false;
    }

    // Helper: Check if identifier is a position variable (AT variable)
    bool isPositionVariable(const std::string& name) const {
        for (const auto& forClause : for_clauses) {
            if (forClause.has_position && forClause.position_var == name) {
                return true;
            }
        }
        return false;
    }
};

} // namespace expocli

#endif // AST_H
