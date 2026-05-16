#!/bin/bash
#
# Check for floating-point symbol contamination in ELF binary
# Returns non-zero if any float/double software emulation symbols are found
#

set -e

ELF_FILE="${1:-build/DutyCycle.elf}"

if [ ! -f "${ELF_FILE}" ]; then
    echo "ERROR: ELF file not found: ${ELF_FILE}"
    echo "Usage: $0 [elf_file]"
    exit 1
fi

echo "Checking: ${ELF_FILE}"

# Match float/double soft-emulation and parsing symbols
PATTERN=" [tT] .*(__aeabi_[df]|strtof|strtod|__add[ds]f|__sub[ds]f|__mul[ds]f|__div[ds]f)"

MATCHES=$(arm-none-eabi-nm "${ELF_FILE}" | grep -E "${PATTERN}" || true)

if [ -n "${MATCHES}" ]; then
    echo "FAIL: Floating-point symbols found in binary:"
    echo "${MATCHES}"
    exit 1
fi

echo "PASS: No floating-point symbols in binary"
