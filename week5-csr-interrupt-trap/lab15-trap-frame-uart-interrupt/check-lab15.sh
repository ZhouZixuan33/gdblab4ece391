#!/usr/bin/env bash
set -eu

LAB_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${LAB_DIR}"

CROSS="${CROSS:-riscv64-unknown-elf}"
NM="${NM:-${CROSS}-nm}"
READELF="${READELF:-${CROSS}-readelf}"
MODE="${MODE:-solution}"
BUILD="build/${MODE}"

kernel_symbols="
_start
before_mret
supervisor_entry
elf_load_done
before_user_sret
supervisor_trap_entry
trap_frame_saved
supervisor_trap_dispatch
before_trap_sret
plic_claim
plic_complete
uart_receive_interrupt
kernel_exit_success
kernel_exit_failure
"

user_symbols="
_user_start
user_regcheck_ecall
user_wait_for_uart
user_after_uart
user_exit_ecall
user_fail
"

for symbol in ${kernel_symbols}; do
    if ! "${NM}" "${BUILD}/kernel.elf" | awk '{print $3}' | grep -Fxq "${symbol}"; then
        echo "Missing kernel checkpoint: ${symbol}"
        exit 1
    fi
done

for symbol in ${user_symbols}; do
    if ! "${NM}" "${BUILD}/user.elf" | awk '{print $3}' | grep -Fxq "${symbol}"; then
        echo "Missing user checkpoint: ${symbol}"
        exit 1
    fi
done

entry="$("${READELF}" -h "${BUILD}/user.elf" | awk '/Entry point address:/ {print $4}')"
if [ "${entry}" != "0x80400000" ]; then
    echo "Expected user entry 0x80400000, found ${entry:-missing}"
    exit 1
fi

load_count="$("${READELF}" -l "${BUILD}/user.elf" | grep -c 'LOAD')"
if [ "${load_count}" -lt 2 ]; then
    echo "user.elf must contain text and shared-flag PT_LOAD segments"
    exit 1
fi

echo "Lab 15 static checks passed."
