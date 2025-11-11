#!/bin/bash
# ExpoCLI Hard Stress Test
# This test pushes expocli to its limits with:
# - Complex nested XSD schema
# - 100 generated XML files
# - Complex SELECT queries with deep nesting
# - Validation of results

set -e

# Source helper functions
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/test_helpers.sh"

# Test-specific directories
HARD_TEST_DIR="$TEST_OUTPUT_DIR/hard_test"
HARD_TEST_DATA="$HARD_TEST_DIR/data"
HARD_TEST_SCHEMA="tests/schemas/enterprise_system.xsd"

# Initialize
print_category "HARD STRESS TEST - Pushing ExpoCLI to the Limit"

echo -e "${COLOR_CYAN}This test will:${COLOR_RESET}"
echo "  1. Use a complex XSD with deeply nested structures"
echo "  2. Generate 100 XML files with random data"
echo "  3. Execute complex SELECT queries across all files"
echo "  4. Validate results for correctness"
echo ""

# Create test directories
rm -rf "$HARD_TEST_DIR"
mkdir -p "$HARD_TEST_DATA"

# ============================================================================
# PHASE 1: Generate 100 XML files
# ============================================================================
echo -e "${COLOR_BOLD}${COLOR_YELLOW}Phase 1: Generating 100 XML files...${COLOR_RESET}"

GENERATION_START=$(date +%s)
echo -e "SET XSD $HARD_TEST_SCHEMA;\nSET DEST $HARD_TEST_DATA;\nGENERATE XML 100 PREFIX enterprise_;\nexit;" | $EXPOCLI_BIN > "$HARD_TEST_DIR/generation.log" 2>&1

if [ $? -ne 0 ]; then
    echo -e "${COLOR_RED}✗ Failed to generate XML files${COLOR_RESET}"
    cat "$HARD_TEST_DIR/generation.log"
    exit 1
fi

GENERATION_END=$(date +%s)
GENERATION_TIME=$((GENERATION_END - GENERATION_START))

# Verify files were created
FILE_COUNT=$(ls -1 "$HARD_TEST_DATA"/enterprise_*.xml 2>/dev/null | wc -l)
echo -e "${COLOR_GREEN}✓ Generated $FILE_COUNT XML files in ${GENERATION_TIME}s${COLOR_RESET}"

if [ "$FILE_COUNT" -ne 100 ]; then
    echo -e "${COLOR_RED}✗ Expected 100 files, got $FILE_COUNT${COLOR_RESET}"
    exit 1
fi

# ============================================================================
# PHASE 2: Execute Complex SELECT Queries
# ============================================================================
echo ""
echo -e "${COLOR_BOLD}${COLOR_YELLOW}Phase 2: Executing complex SELECT queries...${COLOR_RESET}"

# Query 1: Select company names from all files
run_test "HARD-001" \
    "Select company names from 100 files" \
    "SELECT enterprise.company.name FROM \"$HARD_TEST_DATA/\";" \
    ".*"

# Query 2: Select department names with deep nesting
run_test "HARD-002" \
    "Select all department names" \
    "SELECT enterprise.departments.department.name FROM \"$HARD_TEST_DATA/\";" \
    ".*"

# Query 3: Select employee IDs (3 levels deep)
run_test "HARD-003" \
    "Select employee IDs (nested 3 levels)" \
    "SELECT enterprise.departments.department.employees.employee.id FROM \"$HARD_TEST_DATA/\";" \
    ".*"

# Query 4: Select skill names (4 levels deep)
run_test "HARD-004" \
    "Select skill names (nested 4 levels)" \
    "SELECT enterprise.departments.department.employees.employee.skills.skill.name FROM \"$HARD_TEST_DATA/\";" \
    ".*"

# Query 5: WHERE clause on nested field
run_test "HARD-005" \
    "WHERE on nested employee salary" \
    "SELECT enterprise.departments.department.employees.employee.name FROM \"$HARD_TEST_DATA/\" WHERE enterprise.departments.department.employees.employee.salary > 50000;" \
    ".*"

# Query 6: WHERE on deeply nested boolean
run_test "HARD-006" \
    "WHERE on skill certification (4 levels)" \
    "SELECT enterprise.departments.department.employees.employee.skills.skill.name FROM \"$HARD_TEST_DATA/\" WHERE enterprise.departments.department.employees.employee.skills.skill.certified = true;" \
    ".*"

# Query 7: Complex AND condition
run_test "HARD-007" \
    "Complex AND with nested fields" \
    "SELECT enterprise.departments.department.name FROM \"$HARD_TEST_DATA/\" WHERE enterprise.departments.department.budget > 100000 AND enterprise.departments.department.location IS NOT NULL;" \
    ".*"

# Query 8: Product queries with nested features
run_test "HARD-008" \
    "Select product names with price filter" \
    "SELECT enterprise.products.product.name FROM \"$HARD_TEST_DATA/\" WHERE enterprise.products.product.price < 1000;" \
    ".*"

# Query 9: Nested list within product (features)
run_test "HARD-009" \
    "Select product features (nested list)" \
    "SELECT enterprise.products.product.features.feature.description FROM \"$HARD_TEST_DATA/\";" \
    ".*"

# Query 10: Review ratings (nested list)
run_test "HARD-010" \
    "Select review ratings" \
    "SELECT enterprise.products.product.reviews.review.rating FROM \"$HARD_TEST_DATA/\";" \
    ".*"

# Query 11: LIKE operator on nested field
run_test "HARD-011" \
    "LIKE pattern on nested employee position" \
    "SELECT enterprise.departments.department.employees.employee.name FROM \"$HARD_TEST_DATA/\" WHERE enterprise.departments.department.employees.employee.position LIKE /.*Manager.*/;" \
    ".*"

# Query 12: ORDER BY on nested field
run_test "HARD-012" \
    "ORDER BY nested salary field" \
    "SELECT enterprise.departments.department.employees.employee.name FROM \"$HARD_TEST_DATA/\" ORDER BY enterprise.departments.department.employees.employee.salary LIMIT 10;" \
    ".*"

# Query 13: Multiple OR conditions
run_test "HARD-013" \
    "Multiple OR on project priority" \
    "SELECT enterprise.departments.department.projects.project.name FROM \"$HARD_TEST_DATA/\" WHERE enterprise.departments.department.projects.project.priority = 1 OR enterprise.departments.department.projects.project.priority = 2;" \
    ".*"

# Query 14: IS NULL on optional nested field
run_test "HARD-014" \
    "IS NULL on deeply nested milestone" \
    "SELECT enterprise.departments.department.projects.project.name FROM \"$HARD_TEST_DATA/\" WHERE enterprise.departments.department.projects.project.milestones.milestone.title IS NULL;" \
    ".*"

# Query 15: Boolean field selection
run_test "HARD-015" \
    "Select boolean inStock field" \
    "SELECT enterprise.products.product.name FROM \"$HARD_TEST_DATA/\" WHERE enterprise.products.product.inStock = true;" \
    ".*"

# ============================================================================
# PHASE 3: Validation Queries with Result Checking
# ============================================================================
echo ""
echo -e "${COLOR_BOLD}${COLOR_YELLOW}Phase 3: Validation queries with result checking...${COLOR_RESET}"

# Create validation test that counts results
TESTS_TOTAL=$((TESTS_TOTAL + 1))
printf "  %-12s %-45s " "[HARD-V01]" "Count company names = file count"

QUERY_OUTPUT="$HARD_TEST_DIR/validation_company_count.out"
echo -e "SELECT enterprise.company.name FROM \"$HARD_TEST_DATA/\";\nexit;" | $EXPOCLI_BIN > "$QUERY_OUTPUT" 2>&1

# Count non-empty result lines (excluding headers, prompts, status messages, etc.)
# More robust filtering to exclude common status messages
RESULT_COUNT=$(grep -v "^expocli>" "$QUERY_OUTPUT" | \
    grep -v "^SELECT" | \
    grep -v "^$" | \
    grep -v "XSD path" | \
    grep -v "row(s)" | \
    grep -v "file(s)" | \
    grep -v "Query" | \
    grep -v "Processing" | \
    grep -v "Loaded" | \
    grep -v "^─" | \
    grep -v "^═" | \
    grep -v "^│" | \
    grep -v "^║" | \
    grep -E "^[A-Za-z0-9]" | wc -l)

if [ "$RESULT_COUNT" -eq "$FILE_COUNT" ]; then
    TESTS_PASSED=$((TESTS_PASSED + 1))
    echo -e "${COLOR_GREEN}✓ PASS${COLOR_RESET} (found $RESULT_COUNT companies)"
else
    TESTS_FAILED=$((TESTS_FAILED + 1))
    echo -e "${COLOR_RED}✗ FAIL${COLOR_RESET} (expected $FILE_COUNT, got $RESULT_COUNT)"
fi

# Validate that department query returns results from multiple files
TESTS_TOTAL=$((TESTS_TOTAL + 1))
printf "  %-12s %-45s " "[HARD-V02]" "Department query returns results"

QUERY_OUTPUT="$HARD_TEST_DIR/validation_departments.out"
echo -e "SELECT enterprise.departments.department.name FROM \"$HARD_TEST_DATA/\";\nexit;" | $EXPOCLI_BIN > "$QUERY_OUTPUT" 2>&1

DEPT_COUNT=$(grep -v "^expocli>" "$QUERY_OUTPUT" | \
    grep -v "^SELECT" | \
    grep -v "^$" | \
    grep -v "XSD path" | \
    grep -v "row(s)" | \
    grep -v "file(s)" | \
    grep -v "Query" | \
    grep -v "Processing" | \
    grep -v "Loaded" | \
    grep -v "^─" | \
    grep -v "^═" | \
    grep -v "^│" | \
    grep -v "^║" | \
    grep -E "^[A-Za-z0-9]" | wc -l)

if [ "$DEPT_COUNT" -gt 0 ]; then
    TESTS_PASSED=$((TESTS_PASSED + 1))
    echo -e "${COLOR_GREEN}✓ PASS${COLOR_RESET} (found $DEPT_COUNT departments)"
else
    TESTS_FAILED=$((TESTS_FAILED + 1))
    echo -e "${COLOR_RED}✗ FAIL${COLOR_RESET} (no departments found)"
fi

# Validate employee filtering
TESTS_TOTAL=$((TESTS_TOTAL + 1))
printf "  %-12s %-45s " "[HARD-V03]" "Salary filter returns subset"

QUERY_OUTPUT_ALL="$HARD_TEST_DIR/validation_all_employees.out"
QUERY_OUTPUT_FILTERED="$HARD_TEST_DIR/validation_filtered_employees.out"

echo -e "SELECT enterprise.departments.department.employees.employee.name FROM \"$HARD_TEST_DATA/\";\nexit;" | $EXPOCLI_BIN > "$QUERY_OUTPUT_ALL" 2>&1
echo -e "SELECT enterprise.departments.department.employees.employee.name FROM \"$HARD_TEST_DATA/\" WHERE enterprise.departments.department.employees.employee.salary > 80000;\nexit;" | $EXPOCLI_BIN > "$QUERY_OUTPUT_FILTERED" 2>&1

ALL_COUNT=$(grep -v "^expocli>" "$QUERY_OUTPUT_ALL" | \
    grep -v "^SELECT" | \
    grep -v "^$" | \
    grep -v "XSD path" | \
    grep -v "row(s)" | \
    grep -v "file(s)" | \
    grep -v "Query" | \
    grep -v "Processing" | \
    grep -v "Loaded" | \
    grep -v "^─" | \
    grep -v "^═" | \
    grep -v "^│" | \
    grep -v "^║" | \
    grep -E "^[A-Za-z]" | wc -l)

FILTERED_COUNT=$(grep -v "^expocli>" "$QUERY_OUTPUT_FILTERED" | \
    grep -v "^SELECT" | \
    grep -v "^$" | \
    grep -v "XSD path" | \
    grep -v "row(s)" | \
    grep -v "file(s)" | \
    grep -v "Query" | \
    grep -v "Processing" | \
    grep -v "Loaded" | \
    grep -v "^─" | \
    grep -v "^═" | \
    grep -v "^│" | \
    grep -v "^║" | \
    grep -E "^[A-Za-z]" | wc -l)

if [ "$FILTERED_COUNT" -lt "$ALL_COUNT" ] && [ "$FILTERED_COUNT" -gt 0 ]; then
    TESTS_PASSED=$((TESTS_PASSED + 1))
    echo -e "${COLOR_GREEN}✓ PASS${COLOR_RESET} (filtered: $FILTERED_COUNT < all: $ALL_COUNT)"
else
    TESTS_FAILED=$((TESTS_FAILED + 1))
    echo -e "${COLOR_RED}✗ FAIL${COLOR_RESET} (filtered: $FILTERED_COUNT, all: $ALL_COUNT)"
fi

# Validate LIMIT works
TESTS_TOTAL=$((TESTS_TOTAL + 1))
printf "  %-12s %-45s " "[HARD-V04]" "LIMIT reduces result count"

QUERY_OUTPUT="$HARD_TEST_DIR/validation_limit.out"
echo -e "SELECT enterprise.company.name FROM \"$HARD_TEST_DATA/\" LIMIT 5;\nexit;" | $EXPOCLI_BIN > "$QUERY_OUTPUT" 2>&1

LIMITED_COUNT=$(grep -v "^expocli>" "$QUERY_OUTPUT" | \
    grep -v "^SELECT" | \
    grep -v "^$" | \
    grep -v "XSD path" | \
    grep -v "row(s)" | \
    grep -v "file(s)" | \
    grep -v "Query" | \
    grep -v "Processing" | \
    grep -v "Loaded" | \
    grep -v "^─" | \
    grep -v "^═" | \
    grep -v "^│" | \
    grep -v "^║" | \
    grep -E "^[A-Za-z0-9]" | wc -l)

if [ "$LIMITED_COUNT" -eq 5 ]; then
    TESTS_PASSED=$((TESTS_PASSED + 1))
    echo -e "${COLOR_GREEN}✓ PASS${COLOR_RESET} (correctly limited to 5)"
else
    TESTS_FAILED=$((TESTS_FAILED + 1))
    echo -e "${COLOR_RED}✗ FAIL${COLOR_RESET} (expected 5, got $LIMITED_COUNT)"
fi

# Validate boolean filtering
TESTS_TOTAL=$((TESTS_TOTAL + 1))
printf "  %-12s %-45s " "[HARD-V05]" "Boolean filter works correctly"

QUERY_OUTPUT="$HARD_TEST_DIR/validation_boolean.out"
echo -e "SELECT enterprise.products.product.name FROM \"$HARD_TEST_DATA/\" WHERE enterprise.products.product.inStock = true;\nexit;" | $EXPOCLI_BIN > "$QUERY_OUTPUT" 2>&1

# Check that query returns results (filter out status messages first)
BOOL_COUNT=$(grep -v "^expocli>" "$QUERY_OUTPUT" | \
    grep -v "^SELECT" | \
    grep -v "^$" | \
    grep -v "XSD path" | \
    grep -v "row(s)" | \
    grep -v "file(s)" | \
    grep -v "Query" | \
    grep -v "Processing" | \
    grep -v "Loaded" | \
    grep -v "^─" | \
    grep -v "^═" | \
    grep -v "^│" | \
    grep -v "^║" | \
    grep -E "^[A-Za-z0-9]" | wc -l)

if [ "$BOOL_COUNT" -gt 0 ]; then
    TESTS_PASSED=$((TESTS_PASSED + 1))
    echo -e "${COLOR_GREEN}✓ PASS${COLOR_RESET} (boolean query executed)"
else
    TESTS_FAILED=$((TESTS_FAILED + 1))
    echo -e "${COLOR_RED}✗ FAIL${COLOR_RESET} (boolean query failed)"
fi

# ============================================================================
# PHASE 4: Performance Summary
# ============================================================================
echo ""
echo -e "${COLOR_BOLD}${COLOR_YELLOW}Phase 4: Performance Summary${COLOR_RESET}"

# Calculate average query time for a sample query
PERF_START=$(date +%s)
echo -e "SELECT enterprise.company.name FROM \"$HARD_TEST_DATA/\";\nexit;" | $EXPOCLI_BIN > /dev/null 2>&1
PERF_END=$(date +%s)
QUERY_TIME=$((PERF_END - PERF_START))

echo ""
printf "  %-30s %d files\n" "Total XML files generated:" "$FILE_COUNT"
printf "  %-30s %d seconds\n" "Generation time:" "$GENERATION_TIME"
printf "  %-30s %d seconds\n" "Sample query time:" "$QUERY_TIME"
echo ""

# Show disk usage
DISK_USAGE=$(du -sh "$HARD_TEST_DATA" 2>/dev/null | cut -f1)
printf "  %-30s %s\n" "Test data size:" "$DISK_USAGE"
echo ""

# ============================================================================
# Cleanup
# ============================================================================
echo -e "${COLOR_CYAN}Cleaning up test data...${COLOR_RESET}"
rm -rf "$HARD_TEST_DIR"
echo -e "${COLOR_GREEN}✓ Cleanup complete${COLOR_RESET}"
echo ""

# ============================================================================
# Final Status Check
# ============================================================================
if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "${COLOR_RED}✗ Hard stress test completed with $TESTS_FAILED failed validation(s)${COLOR_RESET}"
    echo -e "${COLOR_YELLOW}  Total: $TESTS_TOTAL | Passed: $TESTS_PASSED | Failed: $TESTS_FAILED${COLOR_RESET}"
    echo ""
    exit 1
fi

echo -e "${COLOR_GREEN}✓ Hard stress test completed successfully${COLOR_RESET}"
echo -e "${COLOR_CYAN}  All $TESTS_TOTAL tests passed!${COLOR_RESET}"
echo ""
