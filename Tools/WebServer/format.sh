#!/bin/bash

# MIT License
# Copyright (c) 2025 - 2026 _VIFEXTech

# Auto-format script for DutyCycle WebServer
# Formats all Python, JavaScript, HTML, and CSS files by extension

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}  DutyCycle WebServer Code Formatter${NC}"
echo -e "${BLUE}================================================${NC}"

# Track formatting status
FORMATTED=0
FAILED=0

format_python() {
    echo -e "\n${GREEN}📦 Formatting Python files (*.py)...${NC}"
    
    # Check for black
    if ! command -v black &> /dev/null; then
        echo -e "${YELLOW}   Installing black...${NC}"
        pip install black -q
    fi
    
    # Find all Python files, exclude cache and coverage directories
    local files=$(find . -name "*.py" \
        -not -path "./__pycache__/*" \
        -not -path "./htmlcov/*" \
        -not -path "./.venv/*" \
        -not -path "./venv/*" \
        2>/dev/null | sort)
    
    if [ -z "$files" ]; then
        echo "   No Python files found"
        return
    fi
    
    for file in $files; do
        echo -n "   Formatting $file... "
        if black --quiet --line-length 88 "$file" 2>/dev/null; then
            echo -e "${GREEN}✓${NC}"
            FORMATTED=$((FORMATTED + 1))
        else
            echo -e "${RED}✗${NC}"
            FAILED=$((FAILED + 1))
        fi
    done
}

format_javascript() {
    echo -e "\n${GREEN}📦 Formatting JavaScript files (*.js)...${NC}"
    
    # Find all JS files, exclude node_modules and coverage directories at any level
    local files=$(find . -name "*.js" \
        -not -path "*/node_modules/*" \
        -not -path "*/.venv/*" \
        -not -path "*/htmlcov/*" \
        -not -path "*/coverage-js/*" \
        2>/dev/null | sort)
    
    if [ -z "$files" ]; then
        echo "   No JavaScript files found"
        return
    fi
    
    # Check Node.js version (prettier requires Node >= 14)
    if command -v node &> /dev/null; then
        local node_version=$(node -v 2>/dev/null | sed 's/v//' | cut -d. -f1)
        if [ -n "$node_version" ] && [ "$node_version" -lt 14 ]; then
            echo -e "${YELLOW}   ⚠️  Node.js version too old (need >= 14), skipping JavaScript${NC}"
            return
        fi
    fi
    
    # Try prettier, then npx prettier with auto-install
    local formatter=""
    if command -v prettier &> /dev/null; then
        formatter="prettier"
    elif command -v npx &> /dev/null; then
        echo -e "${YELLOW}   Using npx prettier...${NC}"
        formatter="npx --yes prettier"
    else
        echo -e "${YELLOW}   ⚠️  prettier not found, skipping JavaScript${NC}"
        return
    fi
    
    for file in $files; do
        echo -n "   Formatting $file... "
        if $formatter --write --single-quote "$file" >/dev/null 2>&1; then
            echo -e "${GREEN}✓${NC}"
            FORMATTED=$((FORMATTED + 1))
        else
            echo -e "${RED}✗${NC}"
            FAILED=$((FAILED + 1))
        fi
    done
}

format_html() {
    echo -e "\n${GREEN}📦 Formatting HTML files (*.html)...${NC}"
    
    # Find all HTML files, exclude node_modules and coverage directories at any level
    local files=$(find . -name "*.html" \
        -not -path "*/htmlcov/*" \
        -not -path "*/node_modules/*" \
        -not -path "*/coverage-js/*" \
        2>/dev/null | sort)
    
    if [ -z "$files" ]; then
        echo "   No HTML files found"
        return
    fi
    
    # Check Node.js version
    if command -v node &> /dev/null; then
        local node_version=$(node -v 2>/dev/null | sed 's/v//' | cut -d. -f1)
        if [ -n "$node_version" ] && [ "$node_version" -lt 14 ]; then
            echo -e "${YELLOW}   ⚠️  Node.js version too old (need >= 14), skipping HTML${NC}"
            return
        fi
    fi
    
    local formatter=""
    if command -v prettier &> /dev/null; then
        formatter="prettier"
    elif command -v npx &> /dev/null; then
        formatter="npx --yes prettier"
    else
        echo -e "${YELLOW}   ⚠️  prettier not found, skipping HTML${NC}"
        return
    fi
    
    for file in $files; do
        echo -n "   Formatting $file... "
        if $formatter --write --print-width 120 "$file" >/dev/null 2>&1; then
            echo -e "${GREEN}✓${NC}"
            FORMATTED=$((FORMATTED + 1))
        else
            echo -e "${RED}✗${NC}"
            FAILED=$((FAILED + 1))
        fi
    done
}

format_css() {
    echo -e "\n${GREEN}📦 Formatting CSS files (*.css)...${NC}"
    
    # Find all CSS files, exclude node_modules and coverage directories at any level
    local files=$(find . -name "*.css" \
        -not -path "*/htmlcov/*" \
        -not -path "*/node_modules/*" \
        -not -path "*/coverage-js/*" \
        2>/dev/null | sort)
    
    if [ -z "$files" ]; then
        echo "   No CSS files found"
        return
    fi
    
    # Check Node.js version
    if command -v node &> /dev/null; then
        local node_version=$(node -v 2>/dev/null | sed 's/v//' | cut -d. -f1)
        if [ -n "$node_version" ] && [ "$node_version" -lt 14 ]; then
            echo -e "${YELLOW}   ⚠️  Node.js version too old (need >= 14), skipping CSS${NC}"
            return
        fi
    fi
    
    local formatter=""
    if command -v prettier &> /dev/null; then
        formatter="prettier"
    elif command -v npx &> /dev/null; then
        formatter="npx --yes prettier"
    else
        echo -e "${YELLOW}   ⚠️  prettier not found, skipping CSS${NC}"
        return
    fi
    
    for file in $files; do
        echo -n "   Formatting $file... "
        if $formatter --write "$file" >/dev/null 2>&1; then
            echo -e "${GREEN}✓${NC}"
            FORMATTED=$((FORMATTED + 1))
        else
            echo -e "${RED}✗${NC}"
            FAILED=$((FAILED + 1))
        fi
    done
}

lint_python() {
    echo -e "\n${GREEN}📦 Linting Python files...${NC}"
    
    if ! command -v flake8 &> /dev/null; then
        echo -e "${YELLOW}   Installing flake8...${NC}"
        pip install flake8 -q
    fi
    
    local files=$(find . -name "*.py" \
        -not -path "*/__pycache__/*" \
        -not -path "*/htmlcov/*" \
        -not -path "*/.venv/*" \
        -not -path "*/venv/*" \
        -not -path "*/node_modules/*" \
        -not -path "*/coverage-js/*" \
        2>/dev/null | sort)
    
    if [ -z "$files" ]; then
        echo "   No Python files found"
        return
    fi
    
    local lint_errors=0
    for file in $files; do
        if ! flake8 --ignore=E501,W503,E203 --max-line-length=120 "$file" 2>/dev/null; then
            lint_errors=$((lint_errors + 1))
        fi
    done
    
    if [ $lint_errors -eq 0 ]; then
        echo -e "   ${GREEN}All Python files passed linting ✓${NC}"
    else
        echo -e "   ${YELLOW}$lint_errors file(s) have linting warnings${NC}"
    fi
}

# Parse arguments
CHECK_ONLY=false
LINT=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --check|-c)
            CHECK_ONLY=true
            shift
            ;;
        --lint|-l)
            LINT=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --check, -c    Check formatting without making changes"
            echo "  --lint, -l     Run linting after formatting"
            echo "  --help, -h     Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Run formatters
if [ "$CHECK_ONLY" = true ]; then
    echo -e "${YELLOW}Running in check-only mode...${NC}"
fi

format_python
format_javascript
format_html
format_css

if [ "$LINT" = true ]; then
    lint_python
fi

# Summary
echo -e "\n${BLUE}================================================${NC}"
echo -e "${BLUE}  Summary${NC}"
echo -e "${BLUE}================================================${NC}"
echo -e "   Files formatted: ${GREEN}$FORMATTED${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "   Files failed:    ${RED}$FAILED${NC}"
fi
echo ""

if [ $FAILED -gt 0 ]; then
    exit 1
else
    echo -e "${GREEN}✅ All files formatted successfully!${NC}"
    exit 0
fi
