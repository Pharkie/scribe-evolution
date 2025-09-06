#!/bin/bash

# Scribe Unit Test Runner
# This script runs all unit tests for the Scribe project

echo "🧪 Starting Scribe Unit Tests..."
echo "================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to run a specific test
run_test() {
    local test_name=$1
    echo -e "${YELLOW}Running $test_name...${NC}"
    
    if pio test -e esp32c3-test --filter="*$test_name*" --verbose; then
        echo -e "${GREEN}✅ $test_name PASSED${NC}"
        return 0
    else
        echo -e "${RED}❌ $test_name FAILED${NC}"
        return 1
    fi
}

# Track test results
total_tests=0
passed_tests=0

# Run individual test files
tests=(
    "character_mapping"
    "config_validation" 
    "web_validation"
    "time_utils"
)

for test in "${tests[@]}"; do
    total_tests=$((total_tests + 1))
    if run_test "$test"; then
        passed_tests=$((passed_tests + 1))
    fi
    echo ""
done

# Run all tests together
echo -e "${YELLOW}Running all tests together...${NC}"
total_tests=$((total_tests + 1))
if pio test -e esp32c3-test --verbose; then
    passed_tests=$((passed_tests + 1))
    echo -e "${GREEN}✅ All tests PASSED${NC}"
else
    echo -e "${RED}❌ Some tests FAILED${NC}"
fi

# Summary
echo "================================="
echo -e "Test Summary: ${GREEN}$passed_tests${NC}/${total_tests} test suites passed"

if [ $passed_tests -eq $total_tests ]; then
    echo -e "${GREEN}🎉 All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}🚨 Some tests failed!${NC}"
    exit 1
fi
