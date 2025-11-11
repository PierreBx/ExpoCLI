#!/bin/bash
# Test script for partial path disambiguation

set -e

# Source helper functions
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Use local binary if available
if [ -f "$SCRIPT_DIR/bin/expocli" ]; then
    export EXPOCLI_BIN="$SCRIPT_DIR/bin/expocli"
fi

source "$SCRIPT_DIR/test_helpers.sh"

print_category "PARTIAL PATH DISAMBIGUATION TESTS"

echo "Testing partial path matching with ambiguous attribute names..."
echo "Test data: company.xml with multiple 'name' elements at different levels"
echo ""

TEST_DATA="$SCRIPT_DIR/data/company.xml"

# Test 1: Single component "name" - should match all name elements
run_test "PATH-001" \
    "Single component 'name' (matches all)" \
    "SELECT name FROM \"$TEST_DATA\"" \
    "TechCorp|Engineering|Alice Smith|Bob Johnson|Sales|Carol White|David Brown|Widget Pro|Electronics|Gadget Plus|Software"

# Test 2: Two-component path "company.name" - should match only company name
run_test "PATH-002" \
    "Partial path 'company.name' (company only)" \
    "SELECT company.name FROM \"$TEST_DATA\"" \
    "TechCorp"

# Test 3: Two-component path "department.name" - should match department names
run_test "PATH-003" \
    "Partial path 'department.name' (departments only)" \
    "SELECT department.name FROM \"$TEST_DATA\"" \
    "Engineering|Sales"

# Test 4: Two-component path "employee.name" - should match employee names
run_test "PATH-004" \
    "Partial path 'employee.name' (employees only)" \
    "SELECT employee.name FROM \"$TEST_DATA\"" \
    "Alice Smith|Bob Johnson|Carol White|David Brown"

# Test 5: Two-component path "product.name" - should match product names
run_test "PATH-005" \
    "Partial path 'product.name' (products only)" \
    "SELECT product.name FROM \"$TEST_DATA\"" \
    "Widget Pro|Gadget Plus"

# Test 6: Two-component path "category.name" - should match category names
run_test "PATH-006" \
    "Partial path 'category.name' (categories only)" \
    "SELECT category.name FROM \"$TEST_DATA\"" \
    "Electronics|Software"

# Test 7: Three-component path "department.employee.name" - should match employee names
run_test "PATH-007" \
    "Partial path 'department.employee.name'" \
    "SELECT department.employee.name FROM \"$TEST_DATA\"" \
    "Alice Smith|Bob Johnson|Carol White|David Brown"

# Test 8: Three-component path "product.category.name" - should match category names
run_test "PATH-008" \
    "Partial path 'product.category.name'" \
    "SELECT product.category.name FROM \"$TEST_DATA\"" \
    "Electronics|Software"

# Test 9: WHERE clause with partial path on department.name
run_test "PATH-009" \
    "WHERE with 'department.name = Engineering'" \
    "SELECT department.employee.name FROM \"$TEST_DATA\" WHERE department.name = \"Engineering\"" \
    "Alice Smith|Bob Johnson"

# Test 10: WHERE clause with partial path on employee.salary
run_test "PATH-010" \
    "WHERE with 'employee.salary > 80000'" \
    "SELECT employee.name FROM \"$TEST_DATA\" WHERE employee.salary > 80000" \
    "Alice Smith|Carol White"

# Test 11: WHERE clause with product.name
run_test "PATH-011" \
    "WHERE with 'product.name LIKE /Widget/'" \
    "SELECT product.name FROM \"$TEST_DATA\" WHERE product.name LIKE /.*Widget.*/" \
    "Widget Pro"

# Test 12: Multiple SELECT fields with different partial paths
run_test "PATH-012" \
    "Multiple SELECT fields with partial paths" \
    "SELECT department.name FROM \"$TEST_DATA\"" \
    "Engineering|Sales"

# Test 13: Complex WHERE with department.employee.name
run_test "PATH-013" \
    "Complex WHERE on nested employee" \
    "SELECT employee.name FROM \"$TEST_DATA\" WHERE employee.position LIKE /.*Manager.*/" \
    "Carol White"

# Test 14: SELECT department name and budget
run_test "PATH-014" \
    "SELECT department attributes" \
    "SELECT department.budget FROM \"$TEST_DATA\" WHERE department.name = \"Sales\"" \
    "300000"

# Test 15: SELECT with category within product
run_test "PATH-015" \
    "SELECT nested category within product" \
    "SELECT category.name FROM \"$TEST_DATA\" WHERE product.name = \"Widget Pro\"" \
    "Electronics"

echo ""
echo "Partial path disambiguation tests complete!"
echo ""
