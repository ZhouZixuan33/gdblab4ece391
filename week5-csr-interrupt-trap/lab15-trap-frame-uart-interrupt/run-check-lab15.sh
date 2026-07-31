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
{ sleep 1; printf 'Z'; } | timeout 5s "${QEMU}" \
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

if ! grep -Fq "[S] full trap frame ok" "${OUTPUT}"; then
    echo "Lab 15 did not complete Part A."
    exit 1
fi

if ! grep -Fq "[S] UART interrupt: source 10" "${OUTPUT}"; then
    echo "Lab 15 did not handle the UART interrupt."
    exit 1
fi

if ! grep -Fq "LAB15 PASS" "${OUTPUT}"; then
    echo "Lab 15 did not reach LAB15 PASS."
    exit 1
fi

echo "Lab 15 QEMU run check passed."
