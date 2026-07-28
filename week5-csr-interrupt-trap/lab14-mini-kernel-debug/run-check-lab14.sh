#!/usr/bin/env bash
set -eu

LAB_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${LAB_DIR}"

MODE="${MODE:-solution}"
QEMU="${QEMU:-qemu-system-riscv64}"
BUILD="build/${MODE}"
OUTPUT="${BUILD}/qemu-output.txt"

if [ "${MODE}" != "solution" ]; then
    echo "run-check validates MODE=solution; MODE=${MODE} is intentionally incomplete."
    exit 2
fi

set +e
timeout 5s "${QEMU}" \
    -machine virt \
    -nographic \
    -bios none \
    -kernel "${BUILD}/kernel.elf" >"${OUTPUT}" 2>&1
status=$?
set -e

if [ "${status}" -ne 0 ] && [ "${status}" -ne 124 ]; then
    cat "${OUTPUT}"
    echo "QEMU failed with status ${status}."
    exit "${status}"
fi

cat "${OUTPUT}"

if ! grep -Fq "LAB14 PASS" "${OUTPUT}"; then
    echo "Lab 14 did not reach LAB14 PASS."
    exit 1
fi

echo "Lab 14 QEMU run check passed."
