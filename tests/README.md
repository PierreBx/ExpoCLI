# ExpoCLI Test Suite

Comprehensive test suite for the ExpoCLI XML Query CLI application.

## Overview

This test suite provides systematic testing of all ExpoCLI features, from basic to complex functionality. Tests are organized by feature category and provide clear pass/fail indicators with detailed logging.

## Directory Structure

```
tests/
├── run_tests.sh           # Main test runner script
├── test_helpers.sh        # Helper functions and utilities
├── data/                  # Test XML data files
│   ├── books1.xml
│   ├── books2.xml
│   └── products.xml
├── schemas/               # XSD schema files for validation
│   └── library.xsd
├── output/                # Generated XML files (temporary)
└── logs/                  # Test failure logs
    └── failures.log
```

## Running Tests

### Run All Tests

```bash
./tests/run_tests.sh
```

### Prerequisites

- ExpoCLI binary must be built: `mkdir -p build && cd build && cmake .. && make`
- All test data files must be present in `tests/data/`
- All schema files must be present in `tests/schemas/`

## Test Categories

The test suite covers 13 categories, from simplest to most complex:

### 1. Basic SELECT Queries (5 tests)
- Single field selection
- Multiple field selection
- Nested element paths
- Attribute notation (@attr)
- Special FILE_NAME field

### 2. WHERE Clause - Basic Comparisons (7 tests)
- Equality (=)
- Inequality (!=)
- Less than (<)
- Greater than (>)
- Less than or equal (<=)
- Greater than or equal (>=)
- String comparison

### 3. WHERE Clause - NULL Operators (2 tests)
- IS NULL
- IS NOT NULL

### 4. WHERE Clause - LIKE Operator (4 tests)
- Simple patterns
- Wildcard patterns (regex)
- IS NOT LIKE
- Case-insensitive matching

### 5. WHERE Clause - Logical Operators (5 tests)
- AND operator
- OR operator
- Complex AND/OR combinations
- Parentheses grouping
- Multiple conditions

### 6. ORDER BY (3 tests)
- Numeric field ordering
- String field ordering
- ORDER BY with WHERE

### 7. LIMIT (3 tests)
- Basic LIMIT
- LIMIT with WHERE
- LIMIT with ORDER BY

### 8. Multiple File Queries (2 tests)
- Querying across directories
- FILE_NAME with multiple files

### 9. Configuration Commands (4 tests)
- SET XSD
- SHOW XSD
- SET DEST
- SHOW DEST

### 10. XML Generation (2 tests)
- GENERATE XML with count
- GENERATE XML with custom prefix

### 11. XML Validation (3 tests)
- CHECK single file
- CHECK directory
- CHECK with glob patterns

### 12. Complex Queries (4 tests)
- Multi-feature combinations
- Nested paths with conditions
- NULL with logical operators
- LIKE with AND/OR logic

### 13. Error Handling (3 tests)
- Invalid syntax detection
- Non-existent file handling
- Missing configuration errors

## Test Output

### Successful Run Example

```
╔════════════════════════════════════════════════════════════════╗
║           ExpoCLI Comprehensive Test Suite v1.0               ║
╚════════════════════════════════════════════════════════════════╝

▶ 1. Basic SELECT Queries
──────────────────────────────────────────────────────────────────
  [SELECT-001] Select single field                          ✓ PASS
  [SELECT-002] Select multiple fields                       ✓ PASS
  [SELECT-003] Select from nested elements                  ✓ PASS
  [SELECT-004] Select with attribute notation               ✓ PASS
  [SELECT-005] Select FILE_NAME                             ✓ PASS

...

╔════════════════════════════════════════════════════════════════╗
║                        TEST SUMMARY                            ║
╚════════════════════════════════════════════════════════════════╝

  Total Tests:         47
  Passed:              47
  Failed:              0
  Duration:            12 seconds

  Success Rate: 100%

  ✓ ALL TESTS PASSED
```

### Failed Test Example

```
  [WHERE-001] WHERE with equals                             ✗ FAIL

Failure details logged to: ./tests/logs/failures.log
```

## Test Failure Logs

When tests fail, detailed information is logged to `tests/logs/failures.log`:

```
=== TEST FAILURE: WHERE-001 ===
Description: WHERE with equals
Command: SELECT title FROM tests/data/books1.xml WHERE year = 2020;
Expected pattern: The Great Adventure
Exit code: 0
--- Output ---
(actual output here)
--- Errors ---
(error messages here)
```

## Writing New Tests

### Using run_test Function

```bash
run_test "TEST-ID" \
    "Description of test" \
    "expocli command to run;" \
    "regex pattern to match in output"
```

Example:
```bash
run_test "SELECT-006" \
    "Select with custom field" \
    "SELECT custom.field FROM tests/data/file.xml;" \
    "expected_value"
```

### Test Naming Convention

- **Category prefix**: SELECT, WHERE, NULL, LIKE, LOGIC, ORDER, LIMIT, MULTI, CONFIG, GEN, CHECK, COMPLEX, ERR
- **Number**: 3-digit sequential (001, 002, etc.)
- **Format**: `CATEGORY-NNN`

### Best Practices

1. **One feature per test**: Each test should verify one specific feature
2. **Clear descriptions**: Use descriptive test names that explain what is being tested
3. **Appropriate patterns**: Use regex patterns that are specific enough to verify correctness
4. **Incremental complexity**: Order tests from simple to complex within each category
5. **Clean up**: Remove temporary files after testing

## CI/CD Integration

The test suite is designed for CI/CD integration:

- **Exit code 0**: All tests passed
- **Exit code 1**: One or more tests failed
- **Logs**: Detailed failure information in `tests/logs/failures.log`

### Example CI Configuration

```yaml
test:
  script:
    - mkdir -p build && cd build && cmake .. && make
    - cd .. && ./tests/run_tests.sh
  artifacts:
    when: on_failure
    paths:
      - tests/logs/
      - tests/output/
```

## Maintenance

### Adding Test Data

1. Create XML file in `tests/data/`
2. Ensure valid XML structure
3. Include varied data for testing edge cases

### Adding Test Schemas

1. Create XSD file in `tests/schemas/`
2. Ensure schema is valid
3. Match with corresponding test data

### Cleaning Test Artifacts

```bash
# Clean all temporary files
rm -rf tests/output/* tests/logs/*
```

## Troubleshooting

### Tests Not Running

1. Check binary exists: `ls -l build/expocli`
2. Verify execute permissions: `ls -l tests/run_tests.sh`
3. Check test data files: `ls tests/data/`

### All Tests Failing

1. Rebuild binary: `cd build && make clean && make`
2. Verify data files are not corrupted
3. Check file paths are correct

### Specific Test Failing

1. Review failure log: `cat tests/logs/failures.log`
2. Run command manually to debug
3. Check expected pattern matches actual output

## Contributing

When adding new features to ExpoCLI:

1. Add corresponding test data files
2. Create test cases covering the feature
3. Run full test suite to ensure no regressions
4. Update this README with new test categories

## License

Same as ExpoCLI project license.
