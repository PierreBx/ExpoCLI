# XML Query CLI

A high-performance command-line tool for querying XML files using SQL-like syntax. Built with C++17 and pugixml for efficient XML processing.

## Overview

This tool provides a simple, SQL-like interface for querying XML data. It's designed for users familiar with SQL who need to extract information from multiple large XML files without learning complex tools like xmllint or XPath.

## Features (Phase 1 - MVP)

- **SQL-like Query Syntax**: Familiar SELECT...FROM...WHERE syntax
- **Multiple File Support**: Query across all XML files in a directory
- **Flexible Path Notation**: Use either dot (.) or slash (/) notation for XML paths
- **Comparison Operators**: Support for =, !=, <, >, <=, >=
- **Docker Support**: Consistent build environment via Docker
- **High Performance**: Built with pugixml for efficient XML parsing

## Prerequisites

### Option 1: Using Docker (Recommended)
- Docker installed on your system
- Docker Compose V2 (integrated into Docker CLI)

### Option 2: Local Build
- C++17 compatible compiler (GCC 7+, Clang 5+)
- CMake 3.15+
- pugixml library

## Quick Start

### Using Docker

1. **Build the Docker container:**
```bash
docker compose build
```

2. **Start the container:**
```bash
docker compose run --rm xml-query-cli
```

3. **Inside the container, build the project:**
```bash
mkdir -p build && cd build
cmake ..
make
```

4. **Run queries:**
```bash
./xmlquery "SELECT breakfast_menu/food/name FROM /app/examples WHERE breakfast_menu/food/calories < 500"
```

### Local Build

1. **Install dependencies:**
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libpugixml-dev

# macOS
brew install cmake pugixml
```

2. **Build the project:**
```bash
mkdir build && cd build
cmake ..
make
```

3. **Run queries:**
```bash
./xmlquery "SELECT breakfast_menu/food/name FROM ../examples WHERE breakfast_menu/food/calories < 500"
```

## Query Syntax

### Basic Structure
```sql
SELECT <field>[,<field>...] FROM <path> [WHERE <condition>]
```

### Components

**SELECT clause:**
- Specify one or more fields to extract
- Use `FILE_NAME` to include the source filename
- Separate multiple fields with commas

**FROM clause:**
- Path to XML file or directory
- If directory, all `.xml` files will be queried

**WHERE clause (optional):**
- Filter results based on conditions
- Supports numeric and string comparisons
- Operators: `=`, `!=`, `<`, `>`, `<=`, `>=`

### Field Paths

XML paths can use either notation:
- Dot notation: `breakfast_menu.food.name`
- Slash notation: `breakfast_menu/food/name`

Both notations work identically.

## Examples

### Example 1: Basic Query
**Query:**
```bash
./xmlquery "SELECT breakfast_menu/food/name FROM ./examples"
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

### Example 2: Query with WHERE Clause
**Query:**
```bash
./xmlquery "SELECT breakfast_menu/food/name FROM ./examples WHERE breakfast_menu/food/calories < 500"
```

**Output:**
```
(No results - all breakfast items have >= 600 calories)

0 row(s) returned.
```

### Example 3: Multiple Fields with Filename
**Query:**
```bash
./xmlquery "SELECT FILE_NAME,breakfast_menu/food/name,breakfast_menu/food/price FROM ./examples"
```

**Output:**
```
test.xml | Belgian Waffles | $5.95
test.xml | Strawberry Belgian Waffles | $7.95
test.xml | Berry-Berry Belgian Waffles | $8.95
test.xml | French Toast | $4.50
test.xml | Homestyle Breakfast | $6.95

5 row(s) returned.
```

### Example 4: Query Across Multiple Files
**Query:**
```bash
./xmlquery "SELECT FILE_NAME,lunch_menu/food/name FROM ./examples WHERE lunch_menu/food/calories < 500"
```

**Output:**
```
lunch.xml | Caesar Salad
lunch.xml | Veggie Burger

2 row(s) returned.
```

## Project Structure

```
ExpoCLI/
├── CMakeLists.txt              # Build configuration
├── Dockerfile                  # Docker environment
├── docker-compose.yml          # Docker Compose config
├── README.md                   # This file
│
├── include/                    # Header files
│   ├── parser/
│   │   ├── ast.h              # AST structures and tokens
│   │   ├── lexer.h            # Tokenizer
│   │   └── parser.h           # Query parser
│   ├── executor/
│   │   ├── query_executor.h   # Query execution engine
│   │   └── xml_navigator.h    # XML traversal
│   └── utils/
│       ├── xml_loader.h       # XML file loading
│       └── result_formatter.h # Result output formatting
│
├── src/                        # Implementation files
│   ├── main.cpp               # CLI entry point
│   ├── parser/
│   │   ├── lexer.cpp
│   │   └── parser.cpp
│   ├── executor/
│   │   ├── query_executor.cpp
│   │   └── xml_navigator.cpp
│   └── utils/
│       ├── xml_loader.cpp
│       └── result_formatter.cpp
│
└── examples/                   # Sample XML files
    ├── test.xml               # Breakfast menu
    └── lunch.xml              # Lunch menu
```

## Architecture

The application follows a clean pipeline architecture:

1. **Lexer** (`lexer.cpp`): Tokenizes the query string
2. **Parser** (`parser.cpp`): Builds an Abstract Syntax Tree (AST)
3. **Query Executor** (`query_executor.cpp`): Orchestrates query execution
4. **XML Navigator** (`xml_navigator.cpp`): Traverses XML and evaluates conditions
5. **Result Formatter** (`result_formatter.cpp`): Formats output

## Limitations (Phase 1)

- Single WHERE condition only (no AND/OR)
- No aggregation functions (COUNT, SUM, AVG)
- No ORDER BY or LIMIT clauses
- No JOIN operations
- No wildcard (*) in SELECT

These features are planned for Phase 2 and Phase 3.

## Performance Notes

- Uses pugixml for fast, memory-efficient XML parsing
- Suitable for processing large numbers of large XML files
- Memory usage scales with file size (DOM-based parsing)

## Future Enhancements (Phase 2 & 3)

**Phase 2:**
- Multiple WHERE conditions with AND/OR
- Wildcard support in SELECT
- ORDER BY and LIMIT clauses
- Better error messages

**Phase 3:**
- Aggregation functions (COUNT, AVG, SUM, etc.)
- JOIN operations across files
- Parallel file processing
- XSD validation support
- Index building for repeated queries

## Troubleshooting

### Docker Issues

**Problem:** Permission denied when running docker commands
```bash
# Add your user to docker group
sudo usermod -aG docker $USER
# Log out and back in
```

**Problem:** Port conflicts
```bash
# Use docker compose down to clean up
docker compose down
```

### Build Issues

**Problem:** pugixml not found
```bash
# Ubuntu/Debian
sudo apt-get install libpugixml-dev

# Or build from source
git clone https://github.com/zeux/pugixml.git
cd pugixml
mkdir build && cd build
cmake ..
make
sudo make install
```

**Problem:** C++17 not supported
```bash
# Update your compiler
sudo apt-get install g++-9
export CXX=g++-9
```

## Contributing

This is Phase 1 (MVP). Contributions welcome for:
- Bug fixes
- Performance improvements
- Documentation enhancements
- Additional test cases

## License

MIT License (or specify your license)

## Contact

For issues and questions, please open a GitHub issue.
