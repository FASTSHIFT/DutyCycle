#!/bin/bash
#
# DutyCycle Flash Script
# Flash AT32F421K8U7 using Artery AT-Link
#

set -e

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Flash tool directory
TOOL_DIR="${SCRIPT_DIR}/Artery_ATLINK_Console_Linux-x86_64_V3.0.17"

# Default HEX file path
DEFAULT_HEX="${SCRIPT_DIR}/../Platform/AT32F421/build/DutyCycle.hex"

# Target device
DEVICE="AT32F421K8U7"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_help() {
    echo "Usage: $0 [OPTIONS] [HEX_FILE]"
    echo ""
    echo "Options:"
    echo "  -h, --help      Show this help message"
    echo "  -e, --erase     Erase all flash before programming"
    echo "  -r, --reset     Reset and run after programming"
    echo "  -v, --verify    Verify after programming"
    echo ""
    echo "HEX_FILE: Optional, defaults to ${DEFAULT_HEX}"
    echo ""
    echo "Examples:"
    echo "  $0                    # Flash with default HEX"
    echo "  $0 -e -r -v           # Erase, flash, verify, reset"
    echo "  $0 firmware.hex       # Flash specified file"
}

# Initialize flash tool (create symlinks)
init_tool() {
    if [ ! -d "${TOOL_DIR}" ]; then
        print_error "AT-Link Console not found at: ${TOOL_DIR}"
        print_error "Please copy Artery_ATLINK_Console_Linux-x86_64_V3.0.17/ to Tools/"
        exit 1
    fi

    cd "${TOOL_DIR}"

    for lib in libATLINKLIB.so libATLINKLIB.so.1; do
        if [ ! -e "$lib" ]; then
            ln -sf libATLINKLIB.so.1.0.0 "$lib" 2>/dev/null || true
        fi
    done

    [ ! -e "libQt5Core.so.5" ] && ln -sf libQt5Core.so.5.9.0 libQt5Core.so.5 2>/dev/null || true
    [ ! -e "libicui18n.so.56" ] && ln -sf libicui18n.so.56.1 libicui18n.so.56 2>/dev/null || true
    [ ! -e "libicudata.so.56" ] && ln -sf libicudata.so.56.1 libicudata.so.56 2>/dev/null || true
    [ ! -e "libicuuc.so.56" ] && ln -sf libicuuc.so.56.1 libicuuc.so.56 2>/dev/null || true

    chmod +x ATLink_Console 2>/dev/null || true

    cd - > /dev/null
}

# Parse arguments
ERASE=false
RESET=false
VERIFY=false
HEX_FILE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -e|--erase)
            ERASE=true
            shift
            ;;
        -r|--reset)
            RESET=true
            shift
            ;;
        -v|--verify)
            VERIFY=true
            shift
            ;;
        -*)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
        *)
            HEX_FILE="$1"
            shift
            ;;
    esac
done

# Use default HEX file if not specified
if [ -z "${HEX_FILE}" ]; then
    HEX_FILE="${DEFAULT_HEX}"
fi

# Convert to absolute path
HEX_FILE="$(cd "$(dirname "${HEX_FILE}")" && pwd)/$(basename "${HEX_FILE}")"

# Check HEX file exists
if [ ! -f "${HEX_FILE}" ]; then
    print_error "HEX file not found: ${HEX_FILE}"
    print_error "Build first: cd ../Platform/AT32F421 && cmake -B build && cmake --build build"
    exit 1
fi

print_info "Target device: ${DEVICE}"
print_info "HEX file: ${HEX_FILE}"

# Initialize tool
init_tool

# Build flash command
cd "${TOOL_DIR}"
export LD_LIBRARY_PATH="${TOOL_DIR}"

CMD="./ATLink_Console -device ${DEVICE} -connect"

if [ "${ERASE}" = true ]; then
    CMD="${CMD} -e --all --flash"
    print_info "Full chip erase enabled"
fi

CMD="${CMD} -d --fn ${HEX_FILE}"

if [ "${VERIFY}" = true ]; then
    CMD="${CMD} --v"
    print_info "Verify enabled"
fi

if [ "${RESET}" = true ]; then
    CMD="${CMD} -r"
    print_info "Reset after programming"
fi

print_info "Starting flash..."
echo ""

eval ${CMD}

echo ""
print_info "Flash complete!"
