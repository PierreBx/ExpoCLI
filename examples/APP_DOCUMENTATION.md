# XML Query CLI - Comprehensive Application Documentation

This document provides comprehensive documentation of the XML Query CLI application, demonstrating its capabilities through practical examples, ordered from simplest to most complex.

---

## Example 1: Select Single Field (Basic Query)

### Example Name
**Select Single Field from XML File**

### Description
This is the most basic query that extracts a single field from an XML file. It demonstrates the fundamental SELECT...FROM syntax without any filtering conditions. The query retrieves all `name` elements from the breakfast menu items in the `test.xml` file.

The XML structure being queried:
```xml
<breakfast_menu>
  <food>
    <name>...</name>
    <price>...</price>
    <description>...</description>
    <calories>...</calories>
  </food>
  ...
</breakfast_menu>
```

### Command
```bash
./build/expocli 'SELECT breakfast_menu/food/name FROM "/home/user/ExpoCLI/examples/test.xml"'
```

Or with relative path:
```bash
./build/expocli 'SELECT breakfast_menu/food/name FROM "examples/test.xml"'
```

### Expected Results
Based on the XML content in `test.xml`, we expect to retrieve all 5 food item names from the breakfast menu.

### Actual Results
```
name
---------------------------
Belgian Waffles
Strawberry Belgian Waffles
Berry-Berry Belgian Waffles
French Toast
Homestyle Breakfast

5 rows returned.
```

### Comments
- **Path Structure**: The query uses slash notation (`/`) to navigate the XML hierarchy: `breakfast_menu/food/name`
- **Alternative Notation**: You can also use dot notation: `breakfast_menu.food.name` - both work identically
- **No Filtering**: This query has no WHERE clause, so it returns all matching elements
- **Output Format**: The tool displays results in a columnar format with the field name as header
- **Row Count**: Always shows total number of rows returned at the end
- **Quotes Required**: The file path in the FROM clause must be enclosed in double quotes
- **File vs Directory**: When specifying a file path, only that specific XML file is queried. If you specify a directory path instead, all `.xml` files in that directory would be searched

**Key Takeaway**: This example demonstrates the minimal viable query syntax - SELECT a field FROM a file - which forms the foundation for all more complex queries.

---

