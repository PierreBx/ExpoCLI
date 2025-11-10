# Known Issues - Phase 1 MVP

## Working Features
- ✅ Basic SELECT queries with single field
- ✅ File and directory scanning
- ✅ FILE_NAME special field
- ✅ Lexer and parser for SQL-like syntax
- ✅ pugixml integration via CMake FetchContent
- ✅ Help command

## Issues to Fix

### 1. WHERE Clause Not Working (Priority: HIGH)
**Status:** Not functional
**Description:** WHERE clause queries return no results even when matches should exist.

**Example:**
```bash
./xmlquery 'SELECT breakfast_menu/food/name FROM "examples" WHERE breakfast_menu/food/calories < 700'
# Expected: Belgian Waffles, French Toast
# Actual: No results found
```

**Root Cause:** The query executor's logic for navigating and evaluating WHERE conditions needs refactoring. The issue is in how we calculate the parent path and pass the node context to evaluateCondition.

**Fix Required:** Refactor `QueryExecutor::processFile()` to properly extract the relative path for condition evaluation.

---

### 2. Multi-Field Query Field Ordering Issue (Priority: MEDIUM)
**Status:** Partially working
**Description:** When selecting multiple fields, the output order is inconsistent and fields can appear misaligned.

**Example:**
```bash
./xmlquery 'SELECT FILE_NAME,breakfast_menu/food/name,breakfast_menu/food/calories FROM "examples"'
# Fields appear in wrong order or misaligned
```

**Root Cause:** The result row construction logic in `processFile()` doesn't properly synchronize multiple field results across nodes.

**Fix Required:** Improve the cross-product logic or restructure to process nodes first, then extract all fields from each node.

---

### 3. Path Without Quotes Parsing Issue (Priority: LOW)
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

### 4. Mixed Root Elements Across Files (Priority: LOW)
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
- [x] Single file queries
- [x] Directory scanning
- [x] FILE_NAME field
- [x] Build system (CMake + FetchContent)

### Failed Tests
- [ ] WHERE clause with numeric comparison
- [ ] WHERE clause with string comparison
- [ ] Multi-field SELECT queries

### Not Yet Tested
- [ ] Comparison operators: !=, >, <=, >=
- [ ] String comparisons in WHERE
- [ ] Large file performance
- [ ] Many files in directory
- [ ] Deeply nested XML paths
- [ ] XML with attributes
- [ ] Malformed XML handling

---

## Recommended Next Steps

1. **Fix WHERE clause logic** (Critical for MVP)
   - Refactor `QueryExecutor::processFile()`
   - Add unit tests for WHERE evaluation

2. **Fix multi-field alignment** (Important for usability)
   - Restructure result collection to node-first approach

3. **Add comprehensive error messages**
   - Better parsing errors
   - XML loading errors
   - File not found errors

4. **Add test suite**
   - Unit tests for lexer
   - Unit tests for parser
   - Integration tests for queries

5. **Documentation updates**
   - Update README with "paths must be quoted" requirement
   - Add troubleshooting section
