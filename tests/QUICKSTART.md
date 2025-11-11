# Quick Start Guide - Running Tests

## Running the Test Suite

You can run the test suite from **anywhere** on your system:

```bash
# From project root
./tests/run_tests.sh

# From any directory (using absolute or relative path)
/path/to/ExpoCLI/tests/run_tests.sh

# Or if you're in a different directory
cd /tmp
~/ExpoCLI/tests/run_tests.sh
```

## Binary Detection

The test script is **smart** about finding the ExpoCLI binary:

1. **First**: Checks if `expocli` is in your PATH (installed system-wide)
2. **Then**: Falls back to `./build/expocli` (local build)
3. **Finally**: Offers to build it for you if not found

You'll see which one it's using:
```
Working directory: /home/user/ExpoCLI
Using expocli from PATH: /usr/local/bin/expocli
```

or

```
Working directory: /home/user/ExpoCLI
Using local build: ./build/expocli
```

## First Time Setup

If the binary hasn't been built and isn't in PATH, the test script will:
1. Detect the missing binary
2. Check if you have `cmake` installed
3. Ask if you want to build it automatically
4. Build the project for you (if you answer 'y')

Example:
```
╔════════════════════════════════════════════════════════════════╗
║           ExpoCLI Comprehensive Test Suite v1.0               ║
╚════════════════════════════════════════════════════════════════╝

Working directory: /home/user/ExpoCLI

ERROR: ExpoCLI binary not found
The binary is not in PATH and not found at ./build/expocli

Options:
  1. Install expocli to your PATH, or
  2. Build locally in ./build/

Would you like to build it locally now? (y/n)
```

Type `y` and press Enter, and the build will happen automatically!

## Manual Build

If you prefer to build manually:

```bash
mkdir -p build
cd build
cmake ..
make -j4
```

## What the Test Script Does

1. **Auto-detects project root** - Works from any directory
2. **Shows working directory** - So you know where it's running
3. **Checks for binary** - Verifies expocli is built
4. **Offers auto-build** - Builds project if needed
5. **Runs all tests** - 33 comprehensive tests
6. **Shows results** - Color-coded pass/fail with summary

## Expected Output

When all tests pass:
```
✓ ALL TESTS PASSED

Total Tests:         33
Passed:              33
Failed:              0
Duration:            2 seconds
Success Rate: 100%
```

## Troubleshooting

### "ERROR: Could not find project root directory"
The script couldn't find CMakeLists.txt. Make sure you're running the correct script:
```bash
# Correct
/path/to/ExpoCLI/tests/run_tests.sh

# Not
/path/to/ExpoCLI/run_tests.sh  # Wrong location
```

### "ERROR: ExpoCLI binary not found"
Choose option 'y' to auto-build, or build manually:
```bash
cd /path/to/ExpoCLI
mkdir -p build && cd build && cmake .. && make
```

### Tests fail after code changes
This is normal! The tests are working correctly by detecting issues. Check:
1. The failure log: `tests/logs/failures.log`
2. Individual test output files in `tests/output/`

## Adding to Your Workflow

### Git Hook
Add to `.git/hooks/pre-push`:
```bash
#!/bin/bash
./tests/run_tests.sh || exit 1
```

### CI/CD
```yaml
test:
  script:
    - ./tests/run_tests.sh
  artifacts:
    when: on_failure
    paths:
      - tests/logs/
```

### Make Target
Add to your Makefile:
```make
test:
	./tests/run_tests.sh
```

Then run: `make test`
