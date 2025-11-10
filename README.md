# XML Query CLI

A high-performance command-line tool for querying XML files using SQL-like syntax. Built with C++17 and pugixml for efficient XML processing.

## Overview

This tool provides a simple, SQL-like interface for querying XML data. It's designed for users familiar with SQL who need to extract information from multiple large XML files without learning complex tools like xmllint or XPath.

## Features (Phase 2)

- **Interactive CLI Mode**: REPL interface for running multiple queries in a session
- **Command History**: Navigate previous queries with UP/DOWN arrow keys (last 100 queries)
- **Single Query Mode**: Execute one-off queries from command line
- **SQL-like Query Syntax**: SELECT...FROM...WHERE...ORDER BY...LIMIT syntax
- **Parenthesized Conditions**: Group conditions with parentheses for complex logic
- **Logical Operators**: AND/OR support in WHERE clause with proper precedence
- **Multiple File Support**: Query across all XML files in a directory
- **Flexible Path Notation**: Use either dot (.) or slash (/) notation for XML paths
- **Comparison Operators**: Support for =, !=, <, >, <=, >=
- **ORDER BY**: Sort results numerically or alphabetically
- **LIMIT**: Restrict number of results returned
- **Multi-line Queries**: Semicolon-terminated queries support line breaks
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
- readline library (for command history support)

## Quick Start

### Recommended: Transparent Local Installation

Use expocli as a native command on your machine (it runs in Docker transparently):

1. **Run the installer:**
```bash
./install.sh
```

2. **Reload your shell:**
```bash
source ~/.bashrc  # or ~/.zshrc for zsh users
```

3. **Use expocli from anywhere:**
```bash
expocli                                    # Interactive mode
expocli "SELECT name FROM ./data"          # Single query
```

The first run will automatically build the Docker image and compile expocli (~1-2 minutes). Subsequent runs are instant!

👉 **See [INSTALLATION.md](INSTALLATION.md) for detailed installation instructions and troubleshooting.**

---

### Alternative: Using Docker Directly

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
./expocli "SELECT breakfast_menu/food/name FROM /app/examples WHERE breakfast_menu/food/calories < 500"
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

**Interactive Mode** (recommended for multiple queries):
```bash
./expocli
```

**Single Query Mode**:
```bash
./expocli "SELECT breakfast_menu/food/name FROM ../examples WHERE breakfast_menu/food/calories < 500"
```

## Usage Modes

### Interactive Mode (REPL)

Start interactive mode by running `expocli` without arguments:

```bash
./expocli
```

You'll get a prompt where you can enter multiple queries:

```
XML Query CLI - Phase 2 (Interactive Mode)
Type 'help' for usage information, 'exit' or 'quit' to exit.
Enter SQL-like queries to search XML files.

expocli> SELECT breakfast_menu/food/name FROM /app/examples WHERE breakfast_menu/food/calories < 700
Belgian Waffles
French Toast

2 row(s) returned.

expocli> SELECT bookstore/book/title FROM /app/examples/books.xml ORDER BY price LIMIT 2
Harry Potter
Everyday Italian

2 row(s) returned.

expocli> exit
Bye!
```

**Interactive Commands:**
- `help`, `\h`, `\?` - Show help message
- `exit`, `quit`, `\q` - Exit the program
- `\c`, `clear` - Clear screen
- `\` at end of line - Continue query on next line (multi-line queries)

**Multi-line Query Example:**
```
expocli> SELECT breakfast_menu/food/name \
      ... FROM /app/examples \
      ... WHERE breakfast_menu/food/calories < 700
```

### Single Query Mode

Execute a single query from the command line:

```bash
./expocli "SELECT name FROM /path/to/files WHERE price < 30"
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
./expocli "SELECT breakfast_menu/food/name FROM ./examples"
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
./expocli "SELECT breakfast_menu/food/name FROM ./examples WHERE breakfast_menu/food/calories < 500"
```

**Output:**
```
(No results - all breakfast items have >= 600 calories)

0 row(s) returned.
```

### Example 3: Multiple Fields with Filename
**Query:**
```bash
./expocli "SELECT FILE_NAME,breakfast_menu/food/name,breakfast_menu/food/price FROM ./examples"
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
./expocli "SELECT FILE_NAME,lunch_menu/food/name FROM ./examples WHERE lunch_menu/food/calories < 500"
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

## Upgrading

### After Pulling New Updates

If you've pulled new updates from the repository and encounter compilation errors related to missing headers (like `readline/readline.h`), you need to rebuild the Docker image:

```bash
# Force rebuild the Docker image
docker compose build --no-cache

# Or if using the installer, run it again
./install.sh
```

The installer (`install.sh`) automatically rebuilds the binary with the latest code, but if you're using Docker directly, you must rebuild the image when dependencies change.

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

**Problem:** Compilation error: `readline/readline.h: No such file or directory`
```bash
# This means the Docker image needs to be rebuilt
docker compose build --no-cache

# Then run install.sh again to recompile
./install.sh
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
