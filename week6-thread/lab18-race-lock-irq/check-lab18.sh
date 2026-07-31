#!/usr/bin/env bash
set -eu
MODE="${MODE:-solution}"; ELF="build/$MODE/kernel.elf"
NM="${NM:-riscv64-unknown-elf-nm}"
for s in counter_loaded counter_before_store lock_acquire_observe \
 lock_release_observe irq_save_disable_observe irq_restore_observe; do
 "$NM" "$ELF" | awk '{print $3}' | grep -Fxq "$s" ||
  { echo "Missing checkpoint: $s"; exit 1; }
done
echo "Lab 18 static checks passed."
