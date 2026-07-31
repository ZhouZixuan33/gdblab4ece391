#!/usr/bin/env bash
set -eu
MODE="${MODE:-solution}"
[ "${MODE}" = solution ] || { echo "run-check requires MODE=solution"; exit 2; }
OUT="build/${MODE}/qemu-output.txt"
set +e
timeout 4s "${QEMU:-qemu-system-riscv64}" -machine virt -nographic -bios none \
  -kernel "build/${MODE}/kernel.elf" >"${OUT}" 2>&1
status=$?
set -e
[ "$status" = 124 ] || [ "$status" = 0 ] || { cat "${OUT}"; exit "$status"; }
cat "${OUT}"
grep -Fq "LAB16 PASS" "${OUT}"
echo "Lab 16 QEMU run check passed."
