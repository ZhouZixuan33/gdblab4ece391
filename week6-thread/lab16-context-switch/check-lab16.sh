#!/usr/bin/env bash
set -eu
MODE="${MODE:-solution}"
BUILD="build/${MODE}"
NM="${NM:-riscv64-unknown-elf-nm}"
for symbol in _start _swtch swtch_save_done swtch_tp_changed \
  swtch_restore_done thread_a_before_switch thread_a_after_resume \
  thread_b_after_switch lab16_todo_checkpoint; do
  "${NM}" "${BUILD}/kernel.elf" | awk '{print $3}' | grep -Fxq "${symbol}" ||
    { echo "Missing checkpoint: ${symbol}"; exit 1; }
done
echo "Lab 16 static checks passed."
