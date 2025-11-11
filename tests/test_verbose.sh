#!/bin/bash

# VERBOSE Mode Test Suite
# Tests the ambiguity detection feature

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║           VERBOSE Mode Test Suite                              ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Check if we're running in local mode
if [ -f "$PROJECT_ROOT/build/expocli" ]; then
    EXPOCLI="$PROJECT_ROOT/build/expocli"
    echo -e "${GREEN}Using local build: $EXPOCLI${NC}"
else
    EXPOCLI="expocli"
    echo -e "${YELLOW}Using system expocli${NC}"
fi

cd "$PROJECT_ROOT"

# Test counter
TESTS_PASSED=0
TESTS_FAILED=0

# Helper function to run a test
run_test() {
    local test_name="$1"
    local query="$2"
    local expected_pattern="$3"
    
    echo -n "  [$test_name] "
    
    # Run query with VERBOSE mode
    local output=$(echo -e "SET VERBOSE;\n$query\nexit" | $EXPOCLI 2>&1)
    
    # Check if output matches expected pattern
    if echo "$output" | grep -qE "$expected_pattern"; then
        echo -e "${GREEN}✓ PASS${NC}"
        ((TESTS_PASSED++))
        return 0
    else
        echo -e "${RED}✗ FAIL${NC}"
        echo -e "${YELLOW}Expected pattern: $expected_pattern${NC}"
        echo -e "${YELLOW}Got output:${NC}"
        echo "$output"
        ((TESTS_FAILED++))
        return 1
    fi
}

echo -e "${BLUE}▶ Testing VERBOSE Mode - Ambiguity Detection${NC}"
echo -e "${BLUE}──────────────────────────────────────────────────────────────────────${NC}"

# Test 1: Ambiguous attribute in truly_ambiguous.xml
run_test "VERB-001" \
    'SELECT item.name FROM "./tests/data/truly_ambiguous.xml";' \
    "⚠ Ambiguous attribute\(s\): item\.name"

# Test 2: Non-ambiguous attribute with full path
run_test "VERB-002" \
    'SELECT section.item.name FROM "./tests/data/truly_ambiguous.xml";' \
    "✓ No ambiguous attributes found"

# Test 3: Ambiguous attribute in company_ambiguous.xml
run_test "VERB-003" \
    'SELECT address.street FROM "./tests/data/company_ambiguous.xml";' \
    "⚠ Ambiguous attribute\(s\): address\.street"

# Test 4: Non-ambiguous with specific path
run_test "VERB-004" \
    'SELECT store.address.street FROM "./tests/data/company_ambiguous.xml";' \
    "✓ No ambiguous attributes found"

# Test 5: Top-level attribute (never ambiguous)
run_test "VERB-005" \
    'SELECT name FROM "./tests/data/company_ambiguous.xml";' \
    "✓ No ambiguous attributes found"

# Test 6: Multiple ambiguous attributes
run_test "VERB-006" \
    'SELECT item.name, item.value FROM "./tests/data/truly_ambiguous.xml";' \
    "⚠ Ambiguous attribute\(s\): item\.name, item\.value"

# Test 7: Ambiguous in WHERE clause
run_test "VERB-007" \
    'SELECT name FROM "./tests/data/truly_ambiguous.xml" WHERE item.value > 200;' \
    "⚠ Ambiguous attribute\(s\): item\.value"

# Test 8: Mixed ambiguous and non-ambiguous
run_test "VERB-008" \
    'SELECT section.item.name, item.value FROM "./tests/data/truly_ambiguous.xml";' \
    "⚠ Ambiguous attribute\(s\): item\.value"

echo ""
echo -e "${BLUE}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                        TEST SUMMARY                            ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "  Total Tests:         $((TESTS_PASSED + TESTS_FAILED))"
echo -e "  Passed:              ${GREEN}${TESTS_PASSED}${NC}"
echo -e "  Failed:              ${RED}${TESTS_FAILED}${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}  ✓ ALL VERBOSE MODE TESTS PASSED${NC}"
    exit 0
else
    echo -e "${RED}  ✗ SOME TESTS FAILED${NC}"
    exit 1
fi
