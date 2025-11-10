#!/bin/bash
# Test suite for XML Query CLI

XMLQUERY="../build/xmlquery"
EXAMPLES_DIR="/home/user/ExpoCLI/examples"

echo "========================================="
echo "XML Query CLI - Test Suite"
echo "========================================="
echo ""

# Test 1: Basic SELECT
echo "Test 1: Basic SELECT query"
echo "Query: SELECT breakfast_menu/food/name FROM test.xml"
$XMLQUERY 'SELECT breakfast_menu/food/name FROM "'$EXAMPLES_DIR'/test.xml"'
echo ""

# Test 2: Multi-field SELECT
echo "Test 2: Multi-field SELECT (checking column order)"
echo "Query: SELECT breakfast_menu/food/name,breakfast_menu/food/price,breakfast_menu/food/calories FROM test.xml"
$XMLQUERY 'SELECT breakfast_menu/food/name,breakfast_menu/food/price,breakfast_menu/food/calories FROM "'$EXAMPLES_DIR'/test.xml"'
echo ""

# Test 3: WHERE with numeric comparison
echo "Test 3: WHERE with numeric comparison (<)"
echo "Query: SELECT breakfast_menu/food/name FROM test.xml WHERE breakfast_menu/food/calories < 700"
$XMLQUERY 'SELECT breakfast_menu/food/name FROM "'$EXAMPLES_DIR'/test.xml" WHERE breakfast_menu/food/calories < 700'
echo ""

# Test 4: WHERE with equality
echo "Test 4: WHERE with equality (=)"
echo "Query: SELECT bookstore/book/title FROM books.xml WHERE bookstore/book/year = 2005"
$XMLQUERY 'SELECT bookstore/book/title FROM "'$EXAMPLES_DIR'/books.xml" WHERE bookstore/book/year = 2005'
echo ""

# Test 5: WHERE with string comparison
echo "Test 5: WHERE with string comparison"
echo "Query: SELECT bookstore/book/title FROM books.xml WHERE bookstore/book/author = \"J.K. Rowling\""
$XMLQUERY "SELECT bookstore/book/title FROM \"$EXAMPLES_DIR/books.xml\" WHERE bookstore/book/author = \"J.K. Rowling\""
echo ""

# Test 6: FILE_NAME special field
echo "Test 6: FILE_NAME special field"
echo "Query: SELECT FILE_NAME,breakfast_menu/food/name FROM examples directory"
$XMLQUERY 'SELECT FILE_NAME,breakfast_menu/food/name FROM "'$EXAMPLES_DIR'"'
echo ""

# Test 7: Cross-file query
echo "Test 7: Cross-file query with WHERE"
echo "Query: SELECT FILE_NAME,lunch_menu/food/name FROM examples WHERE lunch_menu/food/calories < 500"
$XMLQUERY 'SELECT FILE_NAME,lunch_menu/food/name FROM "'$EXAMPLES_DIR'" WHERE lunch_menu/food/calories < 500'
echo ""

# Test 8: Greater than comparison
echo "Test 8: Greater than (>) comparison"
echo "Query: SELECT bookstore/book/title,bookstore/book/price FROM books.xml WHERE bookstore/book/price > 35"
$XMLQUERY 'SELECT bookstore/book/title,bookstore/book/price FROM "'$EXAMPLES_DIR'/books.xml" WHERE bookstore/book/price > 35'
echo ""

# Test 9: Less than or equal
echo "Test 9: Less than or equal (<=) comparison"
echo "Query: SELECT breakfast_menu/food/name FROM test.xml WHERE breakfast_menu/food/calories <= 650"
$XMLQUERY 'SELECT breakfast_menu/food/name FROM "'$EXAMPLES_DIR'/test.xml" WHERE breakfast_menu/food/calories <= 650'
echo ""

# Test 10: Not equals
echo "Test 10: Not equals (!=) comparison"
echo "Query: SELECT bookstore/book/title FROM books.xml WHERE bookstore/book/year != 2005"
$XMLQUERY "SELECT bookstore/book/title FROM \"$EXAMPLES_DIR/books.xml\" WHERE bookstore/book/year != 2005"
echo ""

echo "========================================="
echo "All tests completed!"
echo "========================================="
