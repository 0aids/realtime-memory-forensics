#!/usr/bin/env bash
# Written by ai cause i'm lazy.
# I modified it a bit to suit needs.

# --- 1. Determine Number of Cores ---
# Use 'nproc' for Linux, falling back to 1 if not available
CORES=1
if command -v nproc &>/dev/null; then
    CORES=$(nproc)
fi

echo "Detected $CORES cores for parallel compilation."

# --- 2. Build Tests if directory is missing or stale ---
if [ ! -d "build/tests" ]; then
    echo "Build directory 'build/tests' not found. Running full build..."
    make -j "$CORES" tests
    BUILD_STATUS=$?
else
    # Always run make to ensure targets are up-to-date
    echo "Ensuring all tests are built and up-to-date..."
    make -j "$CORES" tests
    BUILD_STATUS=$?
fi

if [ $BUILD_STATUS -ne 0 ]; then
    echo "---------------------------------------------------------"
    echo "ERROR: Test compilation failed (Make returned $BUILD_STATUS)."
    echo "---------------------------------------------------------"
    exit $BUILD_STATUS
fi

echo "---------------------------------------------------------"
echo "Starting test execution..."
echo "---------------------------------------------------------"

SUCCESSES=0
FAILURES=0
TOTAL=0

# --- 3. Execute Tests in Alphanumeric Order ---
# The globbing pattern ensures alphanumeric order by default.
# -x: executable check, -f: file check
for test_file in build/tests/*; do
    if [ -x "$test_file" ] && [ -f "$test_file" ]; then

        TOTAL=$((TOTAL + 1))

        # Execute the test executable
        # Redirect stderr to /dev/null to keep output clean, but show exit status
        "$test_file" 2>/dev/null
        EXIT_CODE=$?

        TEST_NAME=$(basename "$test_file")

        if [ $EXIT_CODE -eq 0 ]; then
            SUCCESSES=$((SUCCESSES + 1))
            echo -e "\033[32m[PASS] $TEST_NAME (Code: $EXIT_CODE)\033[0m"
        else
            FAILURES=$((FAILURES + 1))
            echo -e "\033[31m[FAIL] $TEST_NAME (Code: $EXIT_CODE) \033[0m"
        fi
    fi
done

echo "---------------------------------------------------------"

# --- 4. Calculate and Print Summary ---
if [ $TOTAL -gt 0 ]; then
    echo "--- Test Summary ---"
    echo "Total Tests Run: $TOTAL"
    echo "Successful Tests: $SUCCESSES / $TOTAL"
    echo "Failed Tests: $FAILURES / $TOTAL"
    echo "---------------------------------------------------------"

    # Exit with a non-zero status if any tests failed
    if [ $FAILURES -gt 0 ]; then
        exit 1
    fi
else
    echo "No executable tests found in build/tests/."
    exit 1
fi

exit 0
