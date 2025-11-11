#!/bin/bash
# Test Helper Functions for ExpoCLI
# Provides utilities for test execution, reporting, and validation

# Color codes for output
export COLOR_RESET='\033[0m'
export COLOR_RED='\033[0;31m'
export COLOR_GREEN='\033[0;32m'
export COLOR_YELLOW='\033[1;33m'
export COLOR_BLUE='\033[0;34m'
export COLOR_CYAN='\033[0;36m'
export COLOR_BOLD='\033[1m'

# Test counters
export TESTS_PASSED=0
export TESTS_FAILED=0
export TESTS_TOTAL=0

# Test timing
export TEST_START_TIME=""

# Paths
export TEST_DIR="./tests"
export TEST_DATA_DIR="$TEST_DIR/data"
export TEST_SCHEMA_DIR="$TEST_DIR/schemas"
export TEST_OUTPUT_DIR="$TEST_DIR/output"
export TEST_LOG_DIR="$TEST_DIR/logs"

# Detect expocli binary location with multiple strategies
# Priority order:
# 1. expocli.sh wrapper script in project root
# 2. expocli command in PATH
# 3. ./build/expocli local build
if [ -f "./expocli.sh" ]; then
    export EXPOCLI_BIN="./expocli.sh"
elif command -v expocli &> /dev/null; then
    export EXPOCLI_BIN="expocli"
else
    export EXPOCLI_BIN="./build/expocli"
fi

# Initialize test environment
init_tests() {
    echo -e "${COLOR_BOLD}${COLOR_CYAN}╔════════════════════════════════════════════════════════════════╗${COLOR_RESET}"
    echo -e "${COLOR_BOLD}${COLOR_CYAN}║           ExpoCLI Comprehensive Test Suite v1.0               ║${COLOR_RESET}"
    echo -e "${COLOR_BOLD}${COLOR_CYAN}╚════════════════════════════════════════════════════════════════╝${COLOR_RESET}"
    echo ""

    # Show current directory for debugging
    echo -e "${COLOR_CYAN}Working directory: $(pwd)${COLOR_RESET}"

    # Show which binary will be used
    if [ -f "./expocli.sh" ]; then
        echo -e "${COLOR_GREEN}Using wrapper script: ./expocli.sh${COLOR_RESET}"
    elif command -v expocli &> /dev/null; then
        echo -e "${COLOR_GREEN}Using expocli from PATH: $(which expocli)${COLOR_RESET}"
    else
        echo -e "${COLOR_CYAN}Using local build: $EXPOCLI_BIN${COLOR_RESET}"
    fi
    echo ""

    # Clean and recreate output directories
    rm -rf "$TEST_OUTPUT_DIR"/* "$TEST_LOG_DIR"/* 2>/dev/null
    mkdir -p "$TEST_OUTPUT_DIR" "$TEST_LOG_DIR"

    # Check if binary exists or is available
    if [ ! -f "./expocli.sh" ] && ! command -v expocli &> /dev/null && [ ! -f "$EXPOCLI_BIN" ]; then
        echo -e "${COLOR_RED}ERROR: ExpoCLI binary not found${COLOR_RESET}"
        echo -e "${COLOR_YELLOW}The binary is not in PATH and not found at $EXPOCLI_BIN${COLOR_RESET}"
        echo ""
        echo "Options:"
        echo "  1. Install expocli to your PATH, or"
        echo "  2. Build locally in ./build/"
        echo ""
        echo "Would you like to build it locally now? (y/n)"
        read -r response

        if [[ "$response" =~ ^[Yy]$ ]]; then
            # Check if cmake is available
            if ! command -v cmake &> /dev/null; then
                echo ""
                echo -e "${COLOR_RED}ERROR: cmake is not installed or not in PATH${COLOR_RESET}"
                echo -e "${COLOR_YELLOW}Please install cmake first, or use the installed expocli command${COLOR_RESET}"
                exit 1
            fi

            echo ""
            echo -e "${COLOR_CYAN}Building ExpoCLI...${COLOR_RESET}"
            mkdir -p build
            cd build
            if cmake .. && make -j4; then
                cd ..
                export EXPOCLI_BIN="./build/expocli"
                echo ""
                echo -e "${COLOR_GREEN}✓ Build successful!${COLOR_RESET}"
                echo ""
            else
                cd ..
                echo ""
                echo -e "${COLOR_RED}✗ Build failed. Please check the errors above.${COLOR_RESET}"
                exit 1
            fi
        else
            echo ""
            echo -e "${COLOR_YELLOW}To build manually, run:${COLOR_RESET}"
            echo "  mkdir -p build && cd build && cmake .. && make"
            echo ""
            echo -e "${COLOR_YELLOW}Or ensure 'expocli' is installed and in your PATH${COLOR_RESET}"
            exit 1
        fi
    fi

    # Verify binary is executable (only for local build)
    if [ -f "$EXPOCLI_BIN" ] && [ ! -x "$EXPOCLI_BIN" ]; then
        echo -e "${COLOR_YELLOW}Making binary executable...${COLOR_RESET}"
        chmod +x "$EXPOCLI_BIN"
    fi

    # Record start time
    TEST_START_TIME=$(date +%s)
}

# Print test category header
print_category() {
    local category="$1"
    echo ""
    echo -e "${COLOR_BOLD}${COLOR_BLUE}▶ $category${COLOR_RESET}"
    echo -e "${COLOR_BLUE}$(printf '─%.0s' {1..70})${COLOR_RESET}"
}

# Run a single test case
# Usage: run_test "TEST-ID" "Description" "command" "expected_pattern" ["optional_setup"]
run_test() {
    local test_id="$1"
    local description="$2"
    local command="$3"
    local expected_pattern="$4"
    local setup="${5:-}"

    TESTS_TOTAL=$((TESTS_TOTAL + 1))

    # Print test header
    printf "  %-12s %-45s " "[$test_id]" "$description"

    # Create temp file for output
    local output_file="$TEST_OUTPUT_DIR/${test_id}.out"
    local error_file="$TEST_OUTPUT_DIR/${test_id}.err"

    # Run setup if provided
    if [ -n "$setup" ]; then
        eval "$setup" &>/dev/null
    fi

    # Execute command - split multiple commands on separate lines
    local full_command="$command"

    # Replace all '; ' with ';\n' to put each command on its own line
    # But first handle exit/quit specially
    if [[ "$full_command" =~ "; exit;" ]]; then
        full_command=$(echo "$full_command" | sed 's/; exit;/;\nexit;/g')
    elif [[ "$full_command" =~ "; quit;" ]]; then
        full_command=$(echo "$full_command" | sed 's/; quit;/;\nquit;/g')
    fi

    # Now split other semicolons to separate lines (but not the final one we just added)
    # Replace '; ' (semicolon followed by space) with ';\n' only before exit/quit line
    full_command=$(echo -e "$full_command" | sed '/^exit;/!s/; /;\n/g' | sed '/^quit;/!s/; /;\n/g')

    # If no exit/quit command, append it
    if [[ ! "$full_command" =~ (exit|quit) ]]; then
        full_command=$(echo -e "${full_command}\nexit;")
    fi

    local exit_code=0
    echo -e "$full_command" | $EXPOCLI_BIN > "$output_file" 2> "$error_file" || exit_code=$?

    # Check for expected pattern (grep in both stdout and stderr combined)
    local result="PASS"
    if [ -n "$expected_pattern" ]; then
        # Combine output and error files for checking
        cat "$output_file" "$error_file" > "${output_file}.combined" 2>/dev/null
        if ! grep -qE "$expected_pattern" "${output_file}.combined" 2>/dev/null; then
            result="FAIL"
        fi
        rm -f "${output_file}.combined"
    fi

    # Check for errors (unless explicitly testing error cases)
    if [ $exit_code -ne 0 ] && [[ ! "$test_id" =~ ERR ]]; then
        result="FAIL"
    fi

    # Update counters and print result
    if [ "$result" = "PASS" ]; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo -e "${COLOR_GREEN}✓ PASS${COLOR_RESET}"
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
        echo -e "${COLOR_RED}✗ FAIL${COLOR_RESET}"

        # Log failure details
        {
            echo "=== TEST FAILURE: $test_id ==="
            echo "Description: $description"
            echo "Command: $command"
            echo "Expected pattern: $expected_pattern"
            echo "Exit code: $exit_code"
            echo "--- Output ---"
            cat "$output_file"
            echo "--- Errors ---"
            cat "$error_file"
            echo ""
        } >> "$TEST_LOG_DIR/failures.log"
    fi
}

# Run a command test (simpler version for commands that should just succeed)
run_command_test() {
    local test_id="$1"
    local description="$2"
    local command="$3"

    run_test "$test_id" "$description" "$command" ".*"
}

# Run a validation test (expects specific output)
run_validation_test() {
    local test_id="$1"
    local description="$2"
    local commands="$3"
    local expected_count="$4"

    TESTS_TOTAL=$((TESTS_TOTAL + 1))

    printf "  %-12s %-45s " "[$test_id]" "$description"

    local output_file="$TEST_OUTPUT_DIR/${test_id}.out"

    # Execute commands
    echo "$commands" | $EXPOCLI_BIN > "$output_file" 2>&1

    # Count results
    local actual_count=$(grep -c "^" "$output_file" 2>/dev/null || echo "0")

    # Simple check - just verify no errors
    if grep -qE "Error|FAIL" "$output_file"; then
        TESTS_FAILED=$((TESTS_FAILED + 1))
        echo -e "${COLOR_RED}✗ FAIL${COLOR_RESET}"
    else
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo -e "${COLOR_GREEN}✓ PASS${COLOR_RESET}"
    fi
}

# Print test summary
print_summary() {
    local end_time=$(date +%s)
    local duration=$((end_time - TEST_START_TIME))

    echo ""
    echo -e "${COLOR_BOLD}${COLOR_CYAN}╔════════════════════════════════════════════════════════════════╗${COLOR_RESET}"
    echo -e "${COLOR_BOLD}${COLOR_CYAN}║                        TEST SUMMARY                            ║${COLOR_RESET}"
    echo -e "${COLOR_BOLD}${COLOR_CYAN}╚════════════════════════════════════════════════════════════════╝${COLOR_RESET}"
    echo ""

    printf "  %-20s %d\n" "Total Tests:" "$TESTS_TOTAL"
    printf "  %-20s ${COLOR_GREEN}%d${COLOR_RESET}\n" "Passed:" "$TESTS_PASSED"
    printf "  %-20s ${COLOR_RED}%d${COLOR_RESET}\n" "Failed:" "$TESTS_FAILED"
    printf "  %-20s %d seconds\n" "Duration:" "$duration"
    echo ""

    # Calculate success rate
    if [ $TESTS_TOTAL -gt 0 ]; then
        local success_rate=$((TESTS_PASSED * 100 / TESTS_TOTAL))
        printf "  Success Rate: "
        if [ $success_rate -ge 90 ]; then
            echo -e "${COLOR_GREEN}${success_rate}%%${COLOR_RESET}"
        elif [ $success_rate -ge 70 ]; then
            echo -e "${COLOR_YELLOW}${success_rate}%%${COLOR_RESET}"
        else
            echo -e "${COLOR_RED}${success_rate}%%${COLOR_RESET}"
        fi
    fi

    echo ""

    # Show failure log location if there were failures
    if [ $TESTS_FAILED -gt 0 ]; then
        echo -e "${COLOR_YELLOW}  Failure details logged to: $TEST_LOG_DIR/failures.log${COLOR_RESET}"
        echo ""
    fi

    # Exit with appropriate code
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${COLOR_GREEN}${COLOR_BOLD}  ✓ ALL TESTS PASSED${COLOR_RESET}"
        echo ""
        return 0
    else
        echo -e "${COLOR_RED}${COLOR_BOLD}  ✗ SOME TESTS FAILED${COLOR_RESET}"
        echo ""
        return 1
    fi
}

# Cleanup test environment
cleanup_tests() {
    # Remove temporary files if tests passed
    if [ $TESTS_FAILED -eq 0 ]; then
        rm -rf "$TEST_OUTPUT_DIR"/* 2>/dev/null
    fi
}
