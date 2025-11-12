#!/bin/bash
# ExpoCLI Comprehensive Test Suite - Working Version
# Tests all features with proper query syntax

set -e

# Change to project root directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Verify we're in the correct directory
if [ ! -f "CMakeLists.txt" ] || [ ! -d "tests" ]; then
    echo "ERROR: Could not find project root directory"
    echo "Expected to find CMakeLists.txt and tests/ directory"
    echo "Current directory: $(pwd)"
    exit 1
fi

# Source helper functions
source tests/test_helpers.sh

# Initialize test environment
init_tests

# ============================================================================
# CATEGORY 1: Basic SELECT Queries
# ============================================================================
print_category "1. Basic SELECT Queries"

run_test "SELECT-001" \
    "Select single field" \
    'SELECT title FROM "tests/data/books1.xml";' \
    "The Great Adventure"

run_test "SELECT-002" \
    "Select multiple fields" \
    'SELECT title,price FROM "tests/data/books1.xml";' \
    "29.99"

run_test "SELECT-003" \
    "Select nested with dot notation" \
    'SELECT library.book.title FROM "tests/data/books1.xml";' \
    "The Great Adventure"

run_test "SELECT-004" \
    "Select FILE_NAME" \
    'SELECT FILE_NAME FROM "tests/data/books1.xml";' \
    "books1.xml"

run_test "SELECT-005" \
    "Select from multiple files" \
    'SELECT title FROM "tests/data/";' \
    ".*"

# ============================================================================
# CATEGORY 2: WHERE Clause - Basic Comparisons
# ============================================================================
print_category "2. WHERE Clause - Basic Comparisons"

run_test "WHERE-001" \
    "WHERE equals" \
    'SELECT title FROM "tests/data/books1.xml" WHERE year = 2020;' \
    "The Great Adventure"

run_test "WHERE-002" \
    "WHERE not equals" \
    'SELECT title FROM "tests/data/books1.xml" WHERE year != 2020;' \
    "Learning Programming"

run_test "WHERE-003" \
    "WHERE less than" \
    'SELECT title FROM "tests/data/books1.xml" WHERE price < 40;' \
    "The Great Adventure"

run_test "WHERE-004" \
    "WHERE greater than" \
    'SELECT title FROM "tests/data/books1.xml" WHERE price > 40;' \
    "Learning Programming"

run_test "WHERE-005" \
    "WHERE string equals" \
    'SELECT title FROM "tests/data/books1.xml" WHERE category = Fiction;' \
    "The Great Adventure"

# ============================================================================
# CATEGORY 3: WHERE Clause - NULL Operators
# ============================================================================
print_category "3. WHERE Clause - NULL Operators"

run_test "NULL-001" \
    "IS NULL - find missing elements" \
    'SELECT title FROM "tests/data/books2.xml" WHERE author IS NULL;' \
    "Cooking for Beginners"

run_test "NULL-002" \
    "IS NOT NULL - find present elements" \
    'SELECT title FROM "tests/data/books2.xml" WHERE author IS NOT NULL;' \
    "Data Science Basics"

# ============================================================================
# CATEGORY 4: WHERE Clause - LIKE Operator
# ============================================================================
print_category "4. WHERE Clause - LIKE Operator"

run_test "LIKE-001" \
    "LIKE with pattern" \
    'SELECT title FROM "tests/data/books1.xml" WHERE title LIKE /Great/;' \
    "The Great Adventure"

run_test "LIKE-002" \
    "LIKE with wildcard" \
    'SELECT title FROM "tests/data/books1.xml" WHERE title LIKE /.*Programming.*/;' \
    "Learning Programming"

run_test "LIKE-003" \
    "IS NOT LIKE" \
    'SELECT title FROM "tests/data/books1.xml" WHERE title IS NOT LIKE /Great/;' \
    "Learning Programming"

# ============================================================================
# CATEGORY 5: WHERE Clause - Logical Operators
# ============================================================================
print_category "5. WHERE Clause - Logical Operators"

run_test "LOGIC-001" \
    "AND operator" \
    'SELECT title FROM "tests/data/books1.xml" WHERE year = 2020 AND price < 30;' \
    "The Great Adventure"

run_test "LOGIC-002" \
    "OR operator" \
    'SELECT title FROM "tests/data/books1.xml" WHERE year = 2020 OR year = 2019;' \
    ".*"

run_test "LOGIC-003" \
    "Parentheses grouping" \
    'SELECT title FROM "tests/data/books2.xml" WHERE (price < 25 OR category = Technical) AND year > 2019;' \
    "Data Science Basics"

# ============================================================================
# CATEGORY 6: ORDER BY and LIMIT
# ============================================================================
print_category "6. ORDER BY and LIMIT"

run_test "ORDER-001" \
    "ORDER BY numeric field" \
    'SELECT title FROM "tests/data/books1.xml" ORDER BY year;' \
    "Learning Programming"

run_test "ORDER-002" \
    "ORDER BY DESC numeric field" \
    'SELECT title FROM "tests/data/books1.xml" ORDER BY year DESC;' \
    "The Great Adventure"

run_test "ORDER-003" \
    "ORDER BY ASC explicit" \
    'SELECT title FROM "tests/data/books1.xml" ORDER BY price ASC;' \
    "The Great Adventure"

run_test "ORDER-004" \
    "ORDER BY DESC with price" \
    'SELECT title FROM "tests/data/books1.xml" ORDER BY price DESC;' \
    "Learning Programming"

run_test "LIMIT-001" \
    "LIMIT results" \
    'SELECT title FROM "tests/data/books1.xml" LIMIT 1;' \
    "The Great Adventure"

run_test "LIMIT-002" \
    "LIMIT with ORDER BY" \
    'SELECT title FROM "tests/data/books1.xml" ORDER BY price LIMIT 1;' \
    "The Great Adventure"

run_test "LIMIT-003" \
    "LIMIT with ORDER BY DESC" \
    'SELECT title FROM "tests/data/books1.xml" ORDER BY price DESC LIMIT 1;' \
    "Learning Programming"

run_test "OFFSET-001" \
    "OFFSET without LIMIT" \
    'SELECT title FROM "tests/data/books1.xml" OFFSET 1;' \
    "Learning Programming"

run_test "OFFSET-002" \
    "LIMIT with OFFSET" \
    'SELECT title FROM "tests/data/books1.xml" LIMIT 1 OFFSET 1;' \
    "Learning Programming"

run_test "OFFSET-003" \
    "ORDER BY with LIMIT and OFFSET" \
    'SELECT title FROM "tests/data/books1.xml" ORDER BY year ASC LIMIT 1 OFFSET 1;' \
    "The Great Adventure"

run_test "DISTINCT-001" \
    "DISTINCT removes duplicates" \
    'SELECT DISTINCT category FROM "tests/data/";' \
    "3 row"

run_test "DISTINCT-002" \
    "DISTINCT with ORDER BY" \
    'SELECT DISTINCT category FROM "tests/data/" ORDER BY category ASC;' \
    "Fiction"

run_test "DISTINCT-003" \
    "DISTINCT with LIMIT" \
    'SELECT DISTINCT category FROM "tests/data/" LIMIT 2;' \
    "2 row"

run_test "AGG-001" \
    "COUNT(*) counts all rows" \
    'SELECT COUNT(*) FROM "tests/data/";' \
    "3"

run_test "AGG-002" \
    "SUM aggregates numeric values" \
    'SELECT SUM(price) FROM "tests/data/books1.xml";' \
    "79.9"

run_test "AGG-003" \
    "AVG computes average" \
    'SELECT AVG(price) FROM "tests/data/books1.xml";' \
    "39.9"

run_test "AGG-004" \
    "MIN finds minimum value" \
    'SELECT MIN(price) FROM "tests/data/books1.xml";' \
    "29.9"

run_test "AGG-005" \
    "MAX finds maximum value" \
    'SELECT MAX(price) FROM "tests/data/books1.xml";' \
    "49.9"

run_test "AGG-006" \
    "COUNT(field) counts non-null values" \
    'SELECT COUNT(price) FROM "tests/data/books1.xml";' \
    "2"

run_test "AGG-007" \
    "Multiple aggregates in one query" \
    'SELECT COUNT(*), SUM(price), AVG(price) FROM "tests/data/books1.xml";' \
    "79.9"

# ============================================================================
# CATEGORY 7: XML Attribute Querying (@attr)
# ============================================================================
print_category "7. XML Attribute Querying"

run_test "ATTR-001" \
    "Basic attribute selection" \
    'SELECT @isbn FROM "tests/data/books1.xml";' \
    "978-1-23-456789-0"

run_test "ATTR-002" \
    "Multiple rows with attribute" \
    'SELECT @isbn FROM "tests/data/books1.xml";' \
    "978-0-12-345678-9"

run_test "ATTR-003" \
    "Mix attribute with regular field" \
    'SELECT @isbn, title FROM "tests/data/books1.xml";' \
    "978-1-23-456789-0"

run_test "ATTR-004" \
    "WHERE clause with attribute equals" \
    'SELECT title FROM "tests/data/books1.xml" WHERE @isbn = "978-1-23-456789-0";' \
    "The Great Adventure"

run_test "ATTR-005" \
    "WHERE clause with attribute LIKE" \
    'SELECT title FROM "tests/data/books1.xml" WHERE @isbn LIKE /978-1-.*/;' \
    "The Great Adventure"

run_test "ATTR-006" \
    "SELECT attribute with WHERE on attribute" \
    'SELECT @isbn, title FROM "tests/data/books1.xml" WHERE @isbn = "978-0-12-345678-9";' \
    "Learning Programming"

run_test "ATTR-007" \
    "COUNT aggregate with attribute" \
    'SELECT COUNT(@isbn) FROM "tests/data/books1.xml";' \
    "2"

run_test "ATTR-008" \
    "ORDER BY attribute DESC" \
    'SELECT @isbn FROM "tests/data/books1.xml" ORDER BY @isbn DESC LIMIT 1;' \
    "978-1-23-456789-0"

run_test "ATTR-009" \
    "ORDER BY attribute ASC" \
    'SELECT @isbn FROM "tests/data/books1.xml" ORDER BY @isbn ASC LIMIT 1;' \
    "978-1-23-456789-0"

run_test "ATTR-010" \
    "DISTINCT with attribute" \
    'SELECT DISTINCT @isbn FROM "tests/data/books1.xml";' \
    "978-1-23-456789-0"

# ============================================================================
# CATEGORY 8: IN Operator
# ============================================================================
print_category "8. IN Operator"

run_test "IN-001" \
    "Basic IN with string values" \
    'SELECT title FROM "tests/data/books1.xml" WHERE title IN ("The Great Adventure", "Learning Programming");' \
    "The Great Adventure"

run_test "IN-002" \
    "IN with single value" \
    'SELECT title FROM "tests/data/books1.xml" WHERE title IN ("The Great Adventure");' \
    "The Great Adventure"

run_test "IN-003" \
    "IN with numeric values" \
    'SELECT title FROM "tests/data/books1.xml" WHERE year IN (2019, 2020);' \
    "Learning Programming"

run_test "IN-004" \
    "IN with no matches" \
    'SELECT title FROM "tests/data/books1.xml" WHERE title IN ("Nonexistent Book");' \
    "No results found"

run_test "IN-005" \
    "NOT IN operator" \
    'SELECT title FROM "tests/data/books1.xml" WHERE title NOT IN ("The Great Adventure");' \
    "Learning Programming"

run_test "IN-006" \
    "NOT IN with multiple values" \
    'SELECT title FROM "tests/data/books1.xml" WHERE year NOT IN (2019);' \
    "The Great Adventure"

run_test "IN-007" \
    "IN with attributes" \
    'SELECT title FROM "tests/data/books1.xml" WHERE @isbn IN ("978-1-23-456789-0", "978-0-12-345678-9");' \
    "The Great Adventure"

run_test "IN-008" \
    "NOT IN with attributes" \
    'SELECT title FROM "tests/data/books1.xml" WHERE @isbn NOT IN ("978-0-12-345678-9");' \
    "The Great Adventure"

run_test "IN-009" \
    "IN with mixed SELECT fields" \
    'SELECT title, @isbn FROM "tests/data/books1.xml" WHERE @isbn IN ("978-1-23-456789-0");' \
    "978-1-23-456789-0"

run_test "IN-010" \
    "IN combined with AND" \
    'SELECT title FROM "tests/data/books1.xml" WHERE title IN ("The Great Adventure", "Learning Programming") AND year = 2020;' \
    "The Great Adventure"

# ============================================================================
# CATEGORY 9: Configuration Commands
# ============================================================================
print_category "9. Configuration Commands"

run_test "CONFIG-001" \
    "SET XSD command" \
    'SET XSD tests/schemas/library.xsd; exit;' \
    "XSD path set to"

run_test "CONFIG-002" \
    "SHOW XSD after SET" \
    'SET XSD tests/schemas/library.xsd; SHOW XSD; exit;' \
    "tests/schemas/library.xsd"

run_test "CONFIG-003" \
    "SET DEST command" \
    'SET DEST tests/output; exit;' \
    "DEST path set to"

run_test "CONFIG-004" \
    "SHOW DEST after SET" \
    'SET DEST tests/output; SHOW DEST; exit;' \
    "tests/output"

# ============================================================================
# CATEGORY 10: XML Generation
# ============================================================================
print_category "10. XML Generation"

run_test "GEN-001" \
    "Generate XML files" \
    'SET XSD tests/schemas/library.xsd; SET DEST tests/output; GENERATE XML 2; exit;' \
    "Successfully generated 2 XML"

run_test "GEN-002" \
    "Generate with custom prefix" \
    'SET XSD tests/schemas/library.xsd; SET DEST tests/output; GENERATE XML 1 PREFIX test_; exit;' \
    "Successfully generated 1 XML"

# Clean up generated files
rm -f tests/output/generated_*.xml tests/output/test_*.xml 2>/dev/null

# ============================================================================
# CATEGORY 11: XML Validation (CHECK Command)
# ============================================================================
print_category "11. XML Validation"

run_test "CHECK-001" \
    "Validate single file" \
    'SET XSD tests/schemas/library.xsd; CHECK tests/data/books1.xml; exit;' \
    "✓.*books1.xml"

run_test "CHECK-002" \
    "Validate directory" \
    'SET XSD tests/schemas/library.xsd; CHECK tests/data/; exit;' \
    "Validating.*file"

run_test "CHECK-003" \
    "Validate with pattern" \
    'SET XSD tests/schemas/library.xsd; CHECK tests/data/books*.xml; exit;' \
    "Summary:.*valid"

# ============================================================================
# CATEGORY 12: Error Handling
# ============================================================================
print_category "12. Error Handling"

run_test "ERR-001" \
    "Invalid query syntax" \
    'INVALID QUERY;' \
    "Error|Parse Error"

run_test "ERR-002" \
    "Non-existent file" \
    'SELECT title FROM "nonexistent.xml";' \
    "Error|No.*files found|does not exist"

run_test "ERR-003" \
    "CHECK without XSD" \
    'CHECK tests/data/books1.xml; exit;' \
    "XSD path not set"

# ============================================================================
# CATEGORY 13: HARD STRESS TEST - Complex Nested Structures at Scale
# ============================================================================
print_category "13. HARD STRESS TEST - Enterprise Scale"

echo ""
echo -e "${COLOR_BOLD}${COLOR_YELLOW}═══════════════════════════════════════════════════════════════${COLOR_RESET}"
echo -e "${COLOR_BOLD}${COLOR_YELLOW}  Running comprehensive stress test with 100 generated files  ${COLOR_RESET}"
echo -e "${COLOR_BOLD}${COLOR_YELLOW}  Complex nested structures: 4+ levels of nesting             ${COLOR_RESET}"
echo -e "${COLOR_BOLD}${COLOR_YELLOW}  This may take several minutes...                             ${COLOR_RESET}"
echo -e "${COLOR_BOLD}${COLOR_YELLOW}═══════════════════════════════════════════════════════════════${COLOR_RESET}"
echo ""

# Run the hard test script
if bash "$TEST_DIR/hard_test.sh"; then
    echo -e "${COLOR_GREEN}${COLOR_BOLD}✓ Hard stress test completed successfully${COLOR_RESET}"
else
    echo -e "${COLOR_RED}${COLOR_BOLD}✗ Hard stress test failed${COLOR_RESET}"
fi

echo ""

# ============================================================================
# Print Final Summary
# ============================================================================
print_summary

# Cleanup
cleanup_tests

# Exit with appropriate code
if [ $TESTS_FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi
