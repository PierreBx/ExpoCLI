# Known Issues - Phase 1 MVP

## Working Features
- ✅ Basic SELECT queries with single and multiple fields
- ✅ WHERE clause with all comparison operators (=, !=, <, >, <=, >=)
- ✅ Numeric and string comparisons in WHERE clause
- ✅ File and directory scanning
- ✅ FILE_NAME special field
- ✅ Lexer and parser for SQL-like syntax
- ✅ pugixml integration via CMake FetchContent
- ✅ Help command
- ✅ Cross-file queries (multiple XML files in directory)

## Issues to Fix

### 1. Multi-Field Query Column Ordering Issue (Priority: MEDIUM)
**Status:** Data correct, ordering wrong
**Description:** When selecting multiple fields, columns appear in alphabetical order instead of the order specified in the query.

**Example:**
```bash
./xmlquery 'SELECT breakfast_menu/food/name,breakfast_menu/food/calories FROM "examples"'
# Output: calories | name (alphabetically sorted)
# Expected: name | calories (as specified in query)
```

**Root Cause:** Using `std::map<std::string, std::string>` for ResultRow, which orders keys alphabetically, not by insertion order.

**Fix Required:** Replace ResultRow with `std::vector<std::pair<std::string, std::string>>` or use a custom ordered map structure that preserves insertion order.

---

### 2. Path Without Quotes Parsing Issue (Priority: LOW)
**Status:** Workaround available
**Description:** Paths in FROM clause must be quoted. Paths like `../examples` or `/home/user/data` fail without quotes.

**Example:**
```bash
./xmlquery "SELECT name FROM ../examples"  # FAILS
./xmlquery 'SELECT name FROM "../examples"'  # WORKS
```

**Root Cause:** The lexer tokenizes `/` and `.` as operators, not as part of paths.

**Fix Required:**
- Option 1: Update lexer to recognize filesystem path patterns
- Option 2: Document that paths must be quoted (simpler)

---

### 3. Mixed Root Elements Across Files (Priority: LOW)
**Status:** May cause issues
**Description:** If XML files in a directory have different root elements, queries may produce unexpected results.

**Example:**
- `test.xml` has `<breakfast_menu>`
- `lunch.xml` has `<lunch_menu>`

Querying for `breakfast_menu/food/name` will return empty for `lunch.xml`.

**Fix Required:** This is expected behavior but should be documented clearly.

---

## Testing Checklist

### Completed Tests
- [x] Help command
- [x] Basic SELECT single field
- [x] Multi-field SELECT queries
- [x] Single file queries
- [x] Directory scanning
- [x] FILE_NAME field
- [x] Build system (CMake + FetchContent)
- [x] WHERE clause with numeric comparison (<, >, =, !=, <=, >=)
- [x] WHERE clause with string comparison (=)
- [x] Cross-file queries (breakfast_menu vs lunch_menu)

### Not Yet Tested
- [ ] Large file performance
- [ ] Many files in directory (>10 files)
- [ ] Deeply nested XML paths (>5 levels)
- [ ] XML with attributes
- [ ] Malformed XML handling
- [ ] Edge cases: empty values, missing fields
- [ ] Concurrent file access

---

## Recommended Next Steps

1. **Fix column ordering** (Medium priority for usability)
   - Replace `std::map` with ordered data structure
   - Preserve user-specified field order in output

2. **Add comprehensive error messages**
   - Better parsing errors with line/column information
   - Clearer XML loading errors
   - File not found errors with suggestions

3. **Add test suite**
   - Unit tests for lexer
   - Unit tests for parser
   - Integration tests for queries
   - Performance benchmarks

4. **Documentation updates**
   - Update README with "paths must be quoted" requirement
   - Add more query examples
   - Document limitations and edge cases

5. **Phase 2 Features** (After MVP is stable)
   - Multiple WHERE conditions with AND/OR
   - Wildcard support in SELECT
   - ORDER BY and LIMIT clauses
   - Better error messages
