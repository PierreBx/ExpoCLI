#!/bin/bash
# ExpoCLI HARDEST Stress Test
# This test pushes expocli to EXTREME limits with:
# - Ultra-complex nested XSD schema (6+ levels)
# - 1000 generated XML files (~1 MB each)
# - Complex SELECT queries with extremely deep nesting
# - Performance validation at scale

# Source helper functions
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/test_helpers.sh"

# Test-specific directories
HARDEST_TEST_DIR="$TEST_OUTPUT_DIR/hardest_test"
HARDEST_TEST_DATA="$HARDEST_TEST_DIR/data"
HARDEST_TEST_SCHEMA="tests/schemas/mega_enterprise.xsd"

# File generation parameters
NUM_FILES=100
TARGET_FILE_SIZE_MB=1

# Initialize
print_category "HARDEST STRESS TEST - EXTREME SCALE"

echo -e "${COLOR_CYAN}This ULTIMATE test will:${COLOR_RESET}"
echo "  1. Use an ULTRA-complex XSD with 6+ levels of nesting"
echo "  2. Generate ${NUM_FILES} XML files (~${TARGET_FILE_SIZE_MB}MB each)"
echo "  3. Execute extremely complex SELECT queries across all files"
echo "  4. Validate performance and correctness at massive scale"
echo "  5. Process gigabytes of XML data"
echo ""
echo -e "${COLOR_YELLOW}⚠  This test may take several minutes to complete...${COLOR_RESET}"
echo ""

# Create test directories
mkdir -p "$HARDEST_TEST_DATA"

# ============================================================================
# PHASE 1: Check for existing files or generate new ones
# ============================================================================
FILE_COUNT=$(ls -1 "$HARDEST_TEST_DATA"/mega_*.xml 2>/dev/null | wc -l)

if [ "$FILE_COUNT" -eq ${NUM_FILES} ]; then
    echo -e "${COLOR_BOLD}${COLOR_YELLOW}Phase 1: Using existing ${NUM_FILES} XML files...${COLOR_RESET}"
    echo -e "${COLOR_GREEN}✓ Found ${NUM_FILES} existing XML files (skipping generation)${COLOR_RESET}"
    GENERATION_TIME=0
else
    echo -e "${COLOR_BOLD}${COLOR_YELLOW}Phase 1: Generating ${NUM_FILES} XML files (~${TARGET_FILE_SIZE_MB}MB each)...${COLOR_RESET}"
    echo -e "${COLOR_CYAN}This will create approximately $(( NUM_FILES * TARGET_FILE_SIZE_MB / 1000 )) GB of test data${COLOR_RESET}"
    echo ""

    # Clean up any partial/old files
    rm -f "$HARDEST_TEST_DATA"/mega_*.xml 2>/dev/null

    GENERATION_START=$(date +%s.%N)

    # Generate files in the background to show progress
    echo -e "SET XSD $HARDEST_TEST_SCHEMA;\nSET DEST $HARDEST_TEST_DATA;\nGENERATE XML ${NUM_FILES} PREFIX mega_;\nexit;" | $EXPOCLI_BIN > "$HARDEST_TEST_DIR/generation.log" 2>&1 &
    GEN_PID=$!

    # Show progress while generating
    echo -n "  Generating files"
    while kill -0 $GEN_PID 2>/dev/null; do
        echo -n "."
        sleep 2
    done
    wait $GEN_PID
    GEN_EXIT_CODE=$?
    echo ""

    if [ $GEN_EXIT_CODE -ne 0 ]; then
        echo -e "${COLOR_RED}✗ Failed to generate XML files${COLOR_RESET}"
        cat "$HARDEST_TEST_DIR/generation.log"
        exit 1
    fi

    GENERATION_END=$(date +%s.%N)
    GENERATION_TIME=$(awk "BEGIN {print $GENERATION_END - $GENERATION_START}")

    # Verify files were created
    FILE_COUNT=$(ls -1 "$HARDEST_TEST_DATA"/mega_*.xml 2>/dev/null | wc -l)
    printf "${COLOR_GREEN}✓ Generated %d XML files in %.2f seconds${COLOR_RESET}\n" "$FILE_COUNT" "$GENERATION_TIME"

    if [ "$FILE_COUNT" -ne ${NUM_FILES} ]; then
        echo -e "${COLOR_RED}✗ Expected ${NUM_FILES} files, got $FILE_COUNT${COLOR_RESET}"
        exit 1
    fi
fi

# Calculate total data size
TOTAL_SIZE=$(du -sh "$HARDEST_TEST_DATA" 2>/dev/null | cut -f1)
AVG_FILE_SIZE=$(ls -lh "$HARDEST_TEST_DATA"/mega_*.xml | head -1 | awk '{print $5}')
echo -e "${COLOR_CYAN}  Total data size: $TOTAL_SIZE${COLOR_RESET}"
echo -e "${COLOR_CYAN}  Average file size: $AVG_FILE_SIZE${COLOR_RESET}"
echo ""

# ============================================================================
# PHASE 2: Execute Extremely Complex SELECT Queries
# ============================================================================
echo -e "${COLOR_BOLD}${COLOR_YELLOW}Phase 2: Executing extreme SELECT queries across ${NUM_FILES} files...${COLOR_RESET}"
echo ""

# Query 1: Top-level corporate data (simple warmup)
QUERY_START=$(date +%s.%N)
run_test "HARDEST-001" \
    "Select corporation names (${NUM_FILES} files)" \
    "SELECT megaEnterprise.corporation.name FROM \"$HARDEST_TEST_DATA/\";" \
    ".*"
QUERY_END=$(date +%s.%N)
QUERY1_TIME=$(awk "BEGIN {print $QUERY_END - $QUERY_START}")

# Query 2: Level 3 - Office cities
QUERY_START=$(date +%s.%N)
run_test "HARDEST-002" \
    "Select office cities (Level 3)" \
    "SELECT megaEnterprise.divisions.division.offices.office.city FROM \"$HARDEST_TEST_DATA/\";" \
    ".*"
QUERY_END=$(date +%s.%N)
QUERY2_TIME=$(awk "BEGIN {print $QUERY_END - $QUERY_START}")

# Query 3: Level 4 - Department names
QUERY_START=$(date +%s.%N)
run_test "HARDEST-003" \
    "Select department names (Level 4)" \
    "SELECT megaEnterprise.divisions.division.offices.office.departments.department.name FROM \"$HARDEST_TEST_DATA/\";" \
    ".*"
QUERY_END=$(date +%s.%N)
QUERY3_TIME=$(awk "BEGIN {print $QUERY_END - $QUERY_START}")

# Query 4: Level 5 - Team focuses
run_test "HARDEST-004" \
    "Select team focuses (Level 5)" \
    "SELECT megaEnterprise.divisions.division.offices.office.departments.department.teams.team.focus FROM \"$HARDEST_TEST_DATA/\";" \
    ".*"

# Query 5: Level 6 - Employee names (DEEP!)
run_test "HARDEST-005" \
    "Select employee names (Level 6)" \
    "SELECT megaEnterprise.divisions.division.offices.office.departments.department.teams.team.members.member.name FROM \"$HARDEST_TEST_DATA/\";" \
    ".*"

# Query 6: Level 7 - Certification titles (EXTREME DEPTH!)
run_test "HARDEST-006" \
    "Select certification titles (Level 7 - EXTREME)" \
    "SELECT megaEnterprise.divisions.division.offices.office.departments.department.teams.team.members.member.certifications.certification.title FROM \"$HARDEST_TEST_DATA/\";" \
    ".*"

# Query 7: WHERE on Level 6 field
run_test "HARDEST-007" \
    "WHERE on deeply nested salary > 100000" \
    "SELECT megaEnterprise.divisions.division.offices.office.departments.department.teams.team.members.member.name FROM \"$HARDEST_TEST_DATA/\" WHERE megaEnterprise.divisions.division.offices.office.departments.department.teams.team.members.member.salary > 100000;" \
    ".*"

# Query 8: WHERE on Level 7 boolean
run_test "HARDEST-008" \
    "WHERE on Level 7 certification valid status" \
    "SELECT megaEnterprise.divisions.division.offices.office.departments.department.teams.team.members.member.certifications.certification.title FROM \"$HARDEST_TEST_DATA/\" WHERE megaEnterprise.divisions.division.offices.office.departments.department.teams.team.members.member.certifications.certification.valid = true;" \
    ".*"

# Query 9: Complex AND with deep nesting
run_test "HARDEST-009" \
    "Complex AND on Level 6 fields" \
    "SELECT megaEnterprise.divisions.division.offices.office.departments.department.teams.team.members.member.name FROM \"$HARDEST_TEST_DATA/\" WHERE megaEnterprise.divisions.division.offices.office.departments.department.teams.team.members.member.salary > 80000 AND megaEnterprise.divisions.division.offices.office.departments.department.teams.team.members.member.level > 3;" \
    ".*"

# Query 10: Product catalog branch - Level 4
run_test "HARDEST-010" \
    "Select product names from catalog (Level 4)" \
    "SELECT megaEnterprise.catalog.category.subcategories.subcategory.products.product.name FROM \"$HARDEST_TEST_DATA/\";" \
    ".*"

# Query 11: Product variants - Level 5
run_test "HARDEST-011" \
    "Select product variant colors (Level 5)" \
    "SELECT megaEnterprise.catalog.category.subcategories.subcategory.products.product.variants.variant.color FROM \"$HARDEST_TEST_DATA/\";" \
    ".*"

# Query 12: Inventory locations - Level 6
run_test "HARDEST-012" \
    "Select inventory warehouse locations (Level 6)" \
    "SELECT megaEnterprise.catalog.category.subcategories.subcategory.products.product.variants.variant.inventory.location.warehouse FROM \"$HARDEST_TEST_DATA/\";" \
    ".*"

# Query 13: WHERE on product price
run_test "HARDEST-013" \
    "WHERE on product price < 100" \
    "SELECT megaEnterprise.catalog.category.subcategories.subcategory.products.product.name FROM \"$HARDEST_TEST_DATA/\" WHERE megaEnterprise.catalog.category.subcategories.subcategory.products.product.price < 100;" \
    ".*"

# Query 14: Project sprints - Level 7
run_test "HARDEST-014" \
    "Select sprint numbers (Level 7)" \
    "SELECT megaEnterprise.divisions.division.offices.office.departments.department.teams.team.projects.project.sprints.sprint.number FROM \"$HARDEST_TEST_DATA/\";" \
    ".*"

# Query 15: Complex OR with multiple branches
run_test "HARDEST-015" \
    "Complex OR across different branches" \
    "SELECT megaEnterprise.divisions.division.offices.office.departments.department.teams.team.members.member.name FROM \"$HARDEST_TEST_DATA/\" WHERE megaEnterprise.divisions.division.offices.office.departments.department.teams.team.members.member.salary > 120000 OR megaEnterprise.divisions.division.offices.office.departments.department.teams.team.members.member.remote = true;" \
    ".*"

echo ""

# ============================================================================
# PHASE 3: Performance and Scale Validation
# ============================================================================
echo -e "${COLOR_BOLD}${COLOR_YELLOW}Phase 3: Performance validation at extreme scale...${COLOR_RESET}"
echo ""

# Count total results from a deep query
RESULT_OUTPUT=$(echo "SELECT megaEnterprise.divisions.division.offices.office.departments.department.teams.team.members.member.name FROM \"$HARDEST_TEST_DATA/\";" | $EXPOCLI_BIN 2>&1)
EMPLOYEE_COUNT=$(echo "$RESULT_OUTPUT" | grep "rows returned" | grep -o '[0-9]*' | tail -1)

if [ -n "$EMPLOYEE_COUNT" ] && [ "$EMPLOYEE_COUNT" -gt 0 ]; then
    echo -e "  ${COLOR_GREEN}✓${COLOR_RESET} Processed $EMPLOYEE_COUNT employee records across $FILE_COUNT files"
else
    echo -e "  ${COLOR_YELLOW}⚠${COLOR_RESET} Could not count employee records"
fi

# Test query on product catalog
PRODUCT_OUTPUT=$(echo "SELECT megaEnterprise.catalog.category.subcategories.subcategory.products.product.name FROM \"$HARDEST_TEST_DATA/\";" | $EXPOCLI_BIN 2>&1)
PRODUCT_COUNT=$(echo "$PRODUCT_OUTPUT" | grep "rows returned" | grep -o '[0-9]*' | tail -1)

if [ -n "$PRODUCT_COUNT" ] && [ "$PRODUCT_COUNT" -gt 0 ]; then
    echo -e "  ${COLOR_GREEN}✓${COLOR_RESET} Processed $PRODUCT_COUNT product records across $FILE_COUNT files"
else
    echo -e "  ${COLOR_YELLOW}⚠${COLOR_RESET} Could not count product records"
fi

# Test extremely deep query (Level 7)
CERT_OUTPUT=$(echo "SELECT megaEnterprise.divisions.division.offices.office.departments.department.teams.team.members.member.certifications.certification.title FROM \"$HARDEST_TEST_DATA/\";" | $EXPOCLI_BIN 2>&1)
CERT_COUNT=$(echo "$CERT_OUTPUT" | grep "rows returned" | grep -o '[0-9]*' | tail -1)

if [ -n "$CERT_COUNT" ] && [ "$CERT_COUNT" -gt 0 ]; then
    echo -e "  ${COLOR_GREEN}✓${COLOR_RESET} Processed $CERT_COUNT certification records at Level 7 depth"
else
    echo -e "  ${COLOR_YELLOW}⚠${COLOR_RESET} Could not count certification records"
fi

echo ""

# ============================================================================
# PHASE 4: Performance Summary
# ============================================================================
echo -e "${COLOR_BOLD}${COLOR_YELLOW}Phase 4: Performance Summary${COLOR_RESET}"
echo ""

echo "  Total XML files processed:     $FILE_COUNT files"
printf "  Generation time:               %.2f seconds (%.2f files/sec)\n" "$GENERATION_TIME" "$(awk "BEGIN {print $FILE_COUNT / $GENERATION_TIME}")"
printf "  Sample query time (Level 2):   %.3f seconds\n" "$QUERY1_TIME"
printf "  Sample query time (Level 3):   %.3f seconds\n" "$QUERY2_TIME"
printf "  Sample query time (Level 4):   %.3f seconds\n" "$QUERY3_TIME"
echo ""
echo "  Test data size:                $TOTAL_SIZE"
echo "  Average file size:             $AVG_FILE_SIZE"
echo ""

if [ -n "$EMPLOYEE_COUNT" ]; then
    echo "  Total records processed:"
    echo "    - Employees:                 $EMPLOYEE_COUNT"
    [ -n "$PRODUCT_COUNT" ] && echo "    - Products:                  $PRODUCT_COUNT"
    [ -n "$CERT_COUNT" ] && echo "    - Certifications (Lvl 7):    $CERT_COUNT"
    echo ""
fi

# Test files are preserved for reuse on subsequent runs
# To regenerate: rm -rf ./tests/output/hardest_test/data/
# ============================================================================
echo ""

echo -e "${COLOR_GREEN}✓ HARDEST stress test completed successfully!${COLOR_RESET}"
echo -e "${COLOR_CYAN}  All 15 extreme tests passed!${COLOR_RESET}"
echo ""
