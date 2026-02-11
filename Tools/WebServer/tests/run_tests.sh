#!/bin/bash

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

# Run all tests for DutyCycle WebServer

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WEBSERVER_DIR="$(dirname "$SCRIPT_DIR")"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}  DutyCycle WebServer Test Runner${NC}"
echo -e "${BLUE}================================================${NC}"

# Parse arguments
RUN_UNIT=true
RUN_API=false
COVERAGE=false
HTML_REPORT=false
SERVER_PORT=5000
START_SERVER=false

print_help() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --unit, -u         Run unit tests (default)"
    echo "  --api, -a          Run API tests (requires server running)"
    echo "  --all              Run all tests"
    echo "  --coverage, -c     Run with coverage reporting"
    echo "  --html             Generate HTML coverage report"
    echo "  --port PORT        Server port for API tests (default: 5000)"
    echo "  --start-server     Start server before API tests"
    echo "  --help, -h         Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                          # Run unit tests"
    echo "  $0 --api --port 5001        # Run API tests on port 5001"
    echo "  $0 --all --coverage --html  # Run all tests with HTML coverage"
    echo "  $0 --all --start-server --port 5098  # Run all tests, auto-start server"
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --unit|-u)
            RUN_UNIT=true
            shift
            ;;
        --api|-a)
            RUN_API=true
            RUN_UNIT=false
            shift
            ;;
        --all)
            RUN_UNIT=true
            RUN_API=true
            shift
            ;;
        --coverage|-c)
            COVERAGE=true
            shift
            ;;
        --html)
            HTML_REPORT=true
            shift
            ;;
        --port)
            SERVER_PORT=$2
            shift 2
            ;;
        --start-server)
            START_SERVER=true
            shift
            ;;
        --help|-h)
            print_help
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            print_help
            exit 1
            ;;
    esac
done

TOTAL_PASSED=0
TOTAL_FAILED=0

# Run unit tests with pytest
if [ "$RUN_UNIT" = true ]; then
    echo -e "\n${GREEN}▶ Running Unit Tests (pytest)...${NC}\n"
    
    PYTEST_ARGS="-v"
    
    if [ "$COVERAGE" = true ]; then
        # Exclude tests directory from coverage
        PYTEST_ARGS="$PYTEST_ARGS --cov=$WEBSERVER_DIR --cov-report=term"
        PYTEST_ARGS="$PYTEST_ARGS --cov-config=$SCRIPT_DIR/.coveragerc"
        if [ "$HTML_REPORT" = true ]; then
            PYTEST_ARGS="$PYTEST_ARGS --cov-report=html:$SCRIPT_DIR/htmlcov"
        fi
    fi
    
    # Exclude test_api.py from pytest (it's a standalone script)
    python3 -m pytest "$SCRIPT_DIR" $PYTEST_ARGS --ignore="$SCRIPT_DIR/test_api.py" --ignore="$SCRIPT_DIR/js"
    
    UNIT_RESULT=$?
    
    if [ $UNIT_RESULT -eq 0 ]; then
        TOTAL_PASSED=$((TOTAL_PASSED + 1))
    else
        TOTAL_FAILED=$((TOTAL_FAILED + 1))
    fi
fi

# Run API tests
if [ "$RUN_API" = true ]; then
    echo -e "\n${GREEN}▶ Running API Tests...${NC}\n"
    
    SERVER_PID=""
    
    # Start server if requested
    if [ "$START_SERVER" = true ]; then
        echo -e "${YELLOW}Starting server on port $SERVER_PORT...${NC}"
        python3 "$WEBSERVER_DIR/main.py" --port $SERVER_PORT &
        SERVER_PID=$!
        sleep 2  # Wait for server to start
        
        # Check if server started
        if ! kill -0 $SERVER_PID 2>/dev/null; then
            echo -e "${RED}Failed to start server${NC}"
            exit 1
        fi
        echo -e "${GREEN}Server started (PID: $SERVER_PID)${NC}\n"
    fi
    
    # Run API tests
    python3 "$SCRIPT_DIR/test_api.py" --port $SERVER_PORT
    API_RESULT=$?
    
    # Stop server if we started it
    if [ -n "$SERVER_PID" ]; then
        echo -e "\n${YELLOW}Stopping server (PID: $SERVER_PID)...${NC}"
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
    fi
    
    if [ $API_RESULT -eq 0 ]; then
        TOTAL_PASSED=$((TOTAL_PASSED + 1))
    else
        TOTAL_FAILED=$((TOTAL_FAILED + 1))
    fi
fi

# Final summary
echo -e "\n${BLUE}================================================${NC}"
echo -e "${BLUE}  Final Summary${NC}"
echo -e "${BLUE}================================================${NC}"

if [ $TOTAL_FAILED -eq 0 ]; then
    echo -e "${GREEN}✅ All test suites passed!${NC}"
    exit 0
else
    echo -e "${RED}❌ $TOTAL_FAILED test suite(s) failed${NC}"
    exit 1
fi
