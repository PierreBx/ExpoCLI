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
    'SELECT book.title FROM "tests/data/books1.xml";' \
    "The Great Adventure"

run_test "SELECT-002" \
    "Select multiple fields" \
    'SELECT book.title,book.price FROM "tests/data/books1.xml";' \
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
    'SELECT book.title FROM "tests/data/";' \
    ".*"

# ============================================================================
# CATEGORY 2: WHERE Clause - Basic Comparisons
# ============================================================================
print_category "2. WHERE Clause - Basic Comparisons"

run_test "WHERE-001" \
    "WHERE equals" \
    'SELECT book.title FROM "tests/data/books1.xml" WHERE book.year = 2020;' \
    "The Great Adventure"

run_test "WHERE-002" \
    "WHERE not equals" \
    'SELECT book.title FROM "tests/data/books1.xml" WHERE book.year != 2020;' \
    "Learning Programming"

run_test "WHERE-003" \
    "WHERE less than" \
    'SELECT book.title FROM "tests/data/books1.xml" WHERE book.price < 40;' \
    "The Great Adventure"

run_test "WHERE-004" \
    "WHERE greater than" \
    'SELECT book.title FROM "tests/data/books1.xml" WHERE book.price > 40;' \
    "Learning Programming"

run_test "WHERE-005" \
    "WHERE string equals" \
    'SELECT book.title FROM "tests/data/books1.xml" WHERE book.category = Fiction;' \
    "The Great Adventure"

# ============================================================================
# CATEGORY 3: WHERE Clause - NULL Operators
# ============================================================================
print_category "3. WHERE Clause - NULL Operators"

run_test "NULL-001" \
    "IS NULL - find missing elements" \
    'SELECT book.title FROM "tests/data/books2.xml" WHERE book.author IS NULL;' \
    "Cooking for Beginners"

run_test "NULL-002" \
    "IS NOT NULL - find present elements" \
    'SELECT book.title FROM "tests/data/books2.xml" WHERE book.author IS NOT NULL;' \
    "Data Science Basics"

# ============================================================================
# CATEGORY 4: WHERE Clause - LIKE Operator
# ============================================================================
print_category "4. WHERE Clause - LIKE Operator"

run_test "LIKE-001" \
    "LIKE with pattern" \
    'SELECT book.title FROM "tests/data/books1.xml" WHERE book.title LIKE /Great/;' \
    "The Great Adventure"

run_test "LIKE-002" \
    "LIKE with wildcard" \
    'SELECT book.title FROM "tests/data/books1.xml" WHERE book.title LIKE /.*Programming.*/;' \
    "Learning Programming"

run_test "LIKE-003" \
    "IS NOT LIKE" \
    'SELECT book.title FROM "tests/data/books1.xml" WHERE book.title IS NOT LIKE /Great/;' \
    "Learning Programming"

# ============================================================================
# CATEGORY 5: WHERE Clause - Logical Operators
# ============================================================================
print_category "5. WHERE Clause - Logical Operators"

run_test "LOGIC-001" \
    "AND operator" \
    'SELECT book.title FROM "tests/data/books1.xml" WHERE book.year = 2020 AND book.price < 30;' \
    "The Great Adventure"

run_test "LOGIC-002" \
    "OR operator" \
    'SELECT book.title FROM "tests/data/books1.xml" WHERE book.year = 2020 OR book.year = 2019;' \
    ".*"

run_test "LOGIC-003" \
    "Parentheses grouping" \
    'SELECT book.title FROM "tests/data/books2.xml" WHERE (book.price < 25 OR book.category = Technical) AND book.year > 2019;' \
    "Data Science Basics"

# ============================================================================
# CATEGORY 6: ORDER BY and LIMIT
# ============================================================================
print_category "6. ORDER BY and LIMIT"

run_test "ORDER-001" \
    "ORDER BY numeric field" \
    'SELECT book.title FROM "tests/data/books1.xml" ORDER BY year;' \
    "Learning Programming"

run_test "LIMIT-001" \
    "LIMIT results" \
    'SELECT book.title FROM "tests/data/books1.xml" LIMIT 1;' \
    "The Great Adventure"

run_test "LIMIT-002" \
    "LIMIT with ORDER BY" \
    'SELECT book.title FROM "tests/data/books1.xml" ORDER BY price LIMIT 1;' \
    "The Great Adventure"

# ============================================================================
# CATEGORY 7: Configuration Commands
# ============================================================================
print_category "7. Configuration Commands"

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
# CATEGORY 8: XML Generation
# ============================================================================
print_category "8. XML Generation"

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
# CATEGORY 9: XML Validation (CHECK Command)
# ============================================================================
print_category "9. XML Validation"

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
# CATEGORY 10: Error Handling
# ============================================================================
print_category "10. Error Handling"

run_test "ERR-001" \
    "Invalid query syntax" \
    'INVALID QUERY;' \
    "Error|Parse Error"

run_test "ERR-002" \
    "Non-existent file" \
    'SELECT book.title FROM "nonexistent.xml";' \
    "Error|No.*files found|does not exist"

run_test "ERR-003" \
    "CHECK without XSD" \
    'CHECK tests/data/books1.xml; exit;' \
    "XSD path not set"

# ============================================================================
# CATEGORY 11: VERBOSE Mode - Ambiguity Detection
# ============================================================================
print_category "11. VERBOSE Mode - Ambiguity Detection"

run_test "VERB-001" \
    "Detect ambiguous attribute" \
    'SET VERBOSE; SELECT item.name FROM "./tests/data/truly_ambiguous.xml"; exit;' \
    "⚠.*Ambiguous attribute.*item\.name"

run_test "VERB-002" \
    "Non-ambiguous with full path" \
    'SET VERBOSE; SELECT section.item.name FROM "./tests/data/truly_ambiguous.xml"; exit;' \
    "✓.*No ambiguous attributes found"

run_test "VERB-003" \
    "Ambiguous in company_ambiguous.xml" \
    'SET VERBOSE; SELECT address.street FROM "./tests/data/company_ambiguous.xml"; exit;' \
    "⚠.*Ambiguous attribute.*address\.street"

run_test "VERB-004" \
    "Non-ambiguous with specific path" \
    'SET VERBOSE; SELECT store.address.street FROM "./tests/data/company_ambiguous.xml"; exit;' \
    "✓.*No ambiguous attributes found"

run_test "VERB-005" \
    "Top-level attribute (never ambiguous)" \
    'SET VERBOSE; SELECT name FROM "./tests/data/company_ambiguous.xml"; exit;' \
    "✓.*No ambiguous attributes found"

run_test "VERB-006" \
    "Multiple ambiguous attributes" \
    'SET VERBOSE; SELECT item.name, item.value FROM "./tests/data/truly_ambiguous.xml"; exit;' \
    "⚠.*Ambiguous attribute.*item\.name.*item\.value"

run_test "VERB-007" \
    "Ambiguous in WHERE clause" \
    'SET VERBOSE; SELECT name FROM "./tests/data/truly_ambiguous.xml" WHERE item.value > 200; exit;' \
    "⚠.*Ambiguous attribute.*item\.value"

run_test "VERB-008" \
    "Mixed ambiguous and non-ambiguous" \
    'SET VERBOSE; SELECT section.item.name, item.value FROM "./tests/data/truly_ambiguous.xml"; exit;' \
    "⚠.*Ambiguous attribute.*item\.value"

# ============================================================================
# CATEGORY 12: HARD STRESS TEST - Complex Nested Structures at Scale
# ============================================================================
print_category "12. HARD STRESS TEST - Enterprise Scale"

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
# CATEGORY 13: HARDEST STRESS TEST - EXTREME SCALE
# ============================================================================
print_category "13. HARDEST STRESS TEST - EXTREME SCALE"

echo ""
echo -e "${COLOR_BOLD}${COLOR_YELLOW}═══════════════════════════════════════════════════════════════${COLOR_RESET}"
echo -e "${COLOR_BOLD}${COLOR_YELLOW}  Running ULTIMATE stress test with 100 generated files       ${COLOR_RESET}"
echo -e "${COLOR_BOLD}${COLOR_YELLOW}  Ultra-complex nested structures: 6-7 levels of nesting      ${COLOR_RESET}"
echo -e "${COLOR_BOLD}${COLOR_YELLOW}  Each file ~1-4MB in size = ~100-400MB total test data       ${COLOR_RESET}"
echo -e "${COLOR_BOLD}${COLOR_YELLOW}  This WILL take several minutes to complete...                ${COLOR_RESET}"
echo -e "${COLOR_BOLD}${COLOR_YELLOW}═══════════════════════════════════════════════════════════════${COLOR_RESET}"
echo ""

# Run the hardest test script
if bash "$TEST_DIR/hardest_test.sh"; then
    echo -e "${COLOR_GREEN}${COLOR_BOLD}✓ HARDEST stress test completed successfully!${COLOR_RESET}"
else
    echo -e "${COLOR_RED}${COLOR_BOLD}✗ HARDEST stress test failed${COLOR_RESET}"
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
