# XML Query CLI - Examples

This document provides comprehensive examples of using the XML Query CLI tool.

## Sample Data Files

- `test.xml` - Breakfast menu with food items
- `lunch.xml` - Lunch menu with food items
- `books.xml` - Bookstore with book catalog

## Basic Queries

### 1. Select Single Field

```bash
xmlquery 'SELECT breakfast_menu/food/name FROM "/path/to/examples/test.xml"'
```

**Output:**
```
Belgian Waffles
Strawberry Belgian Waffles
Berry-Berry Belgian Waffles
French Toast
Homestyle Breakfast

5 row(s) returned.
```

### 2. Select Multiple Fields

```bash
xmlquery 'SELECT breakfast_menu/food/name,breakfast_menu/food/price FROM "/path/to/examples/test.xml"'
```

**Output:**
```
Belgian Waffles | $5.95
Strawberry Belgian Waffles | $7.95
Berry-Berry Belgian Waffles | $8.95
French Toast | $4.50
Homestyle Breakfast | $6.95

5 row(s) returned.
```

**Note:** Fields are returned in the order specified in the query!

## WHERE Clause Examples

### 3. Numeric Comparison - Less Than

```bash
xmlquery 'SELECT breakfast_menu/food/name FROM "/path/to/examples/test.xml" WHERE breakfast_menu/food/calories < 700'
```

**Output:**
```
Belgian Waffles
French Toast

2 row(s) returned.
```

### 4. Numeric Comparison - Greater Than

```bash
xmlquery 'SELECT breakfast_menu/food/name FROM "/path/to/examples/test.xml" WHERE breakfast_menu/food/calories > 800'
```

**Output:**
```
Strawberry Belgian Waffles
Berry-Berry Belgian Waffles
Homestyle Breakfast

3 row(s) returned.
```

### 5. Equality Comparison

```bash
xmlquery 'SELECT breakfast_menu/food/name FROM "/path/to/examples/test.xml" WHERE breakfast_menu/food/calories = 650'
```

**Output:**
```
Belgian Waffles

1 row(s) returned.
```

### 6. Not Equals

```bash
xmlquery "SELECT breakfast_menu/food/name FROM \"/path/to/examples/test.xml\" WHERE breakfast_menu/food/calories != 900"
```

**Output:**
```
Belgian Waffles
French Toast
Homestyle Breakfast

3 row(s) returned.
```

**Note:** Use double quotes around the query when using `!=` to prevent shell interpretation.

### 7. String Comparison

```bash
xmlquery 'SELECT breakfast_menu/food/price FROM "/path/to/examples/test.xml" WHERE breakfast_menu/food/name = "French Toast"'
```

**Output:**
```
$4.50

1 row(s) returned.
```

### 8. Less Than or Equal

```bash
xmlquery 'SELECT breakfast_menu/food/name FROM "/path/to/examples/test.xml" WHERE breakfast_menu/food/calories <= 650'
```

**Output:**
```
Belgian Waffles
French Toast

2 row(s) returned.
```

### 9. Greater Than or Equal

```bash
xmlquery 'SELECT breakfast_menu/food/name FROM "/path/to/examples/test.xml" WHERE breakfast_menu/food/calories >= 900'
```

**Output:**
```
Strawberry Belgian Waffles
Berry-Berry Belgian Waffles
Homestyle Breakfast

3 row(s) returned.
```

## Multi-File Queries

### 10. Query All Files in Directory

```bash
xmlquery 'SELECT FILE_NAME,breakfast_menu/food/name FROM "/path/to/examples"'
```

**Output:**
```
test.xml | Belgian Waffles
test.xml | Strawberry Belgian Waffles
test.xml | Berry-Berry Belgian Waffles
test.xml | French Toast
test.xml | Homestyle Breakfast

5 row(s) returned.
```

### 11. Cross-File Query with WHERE

```bash
xmlquery 'SELECT FILE_NAME,lunch_menu/food/name FROM "/path/to/examples" WHERE lunch_menu/food/calories < 500'
```

**Output:**
```
lunch.xml | Caesar Salad
lunch.xml | Veggie Burger

2 row(s) returned.
```

## Special Fields

### 12. Using FILE_NAME

```bash
xmlquery 'SELECT FILE_NAME,bookstore/book/title FROM "/path/to/examples/books.xml"'
```

**Output:**
```
books.xml | Everyday Italian
books.xml | Harry Potter
books.xml | XQuery Kick Start
books.xml | Learning XML

4 row(s) returned.
```

## Advanced Examples

### 13. Multi-Field Query with WHERE

```bash
xmlquery 'SELECT FILE_NAME,breakfast_menu/food/name,breakfast_menu/food/price,breakfast_menu/food/calories FROM "/path/to/examples" WHERE breakfast_menu/food/calories < 700'
```

**Output:**
```
test.xml | Belgian Waffles | $5.95 | 650
test.xml | French Toast | $4.50 | 600

2 row(s) returned.
```

### 14. Books Published After 2003

```bash
xmlquery 'SELECT bookstore/book/title,bookstore/book/year FROM "/path/to/examples/books.xml" WHERE bookstore/book/year > 2003'
```

**Output:**
```
Everyday Italian | 2005
Harry Potter | 2005

2 row(s) returned.
```

### 15. Expensive Books

```bash
xmlquery 'SELECT bookstore/book/title,bookstore/book/price FROM "/path/to/examples/books.xml" WHERE bookstore/book/price > 35'
```

**Output:**
```
XQuery Kick Start | 49.99
Learning XML | 39.95

2 row(s) returned.
```

## Path Notation

Both dot (`.`) and slash (`/`) notation work:

```bash
# These are equivalent:
xmlquery 'SELECT breakfast_menu.food.name FROM "/path/to/examples/test.xml"'
xmlquery 'SELECT breakfast_menu/food/name FROM "/path/to/examples/test.xml"'
```

## Tips and Best Practices

1. **Always quote paths** in the FROM clause:
   - ✅ Good: `FROM "/path/to/files"`
   - ❌ Bad: `FROM /path/to/files`

2. **Use double quotes for queries with special operators:**
   - For `!=`, use: `xmlquery "SELECT ... WHERE field != value"`
   - This prevents shell interpretation of `!`

3. **Column order is preserved:**
   - `SELECT name,price` returns columns in that order
   - `SELECT price,name` returns columns in reverse order

4. **FILE_NAME is a special field:**
   - Always returns the filename, regardless of XML structure
   - Useful for cross-file queries

5. **Paths must match XML structure:**
   - Query path must exactly match the XML hierarchy
   - Case-sensitive

## Common Patterns

### Find all items under a price threshold
```bash
xmlquery 'SELECT name,price FROM "/path/to/file.xml" WHERE price < 10'
```

### Get all items from a specific category
```bash
xmlquery 'SELECT title FROM "/path/to/books.xml" WHERE category = "web"'
```

### List all files containing specific data
```bash
xmlquery 'SELECT FILE_NAME FROM "/path/to/dir" WHERE field = "value"'
```

### Extract multiple related fields
```bash
xmlquery 'SELECT name,description,price FROM "/path/to/file.xml"'
```

## Troubleshooting

### No results found
- Check that the XML path matches your file structure exactly
- Verify the WHERE condition is correct (check data types)
- Ensure paths in FROM clause are quoted

### Parse errors
- Make sure all paths in FROM are quoted
- Use proper operator syntax
- Check for balanced quotes

### Wrong column order
- Column order is preserved as of Phase 2
- Verify you're specifying fields in the desired order
