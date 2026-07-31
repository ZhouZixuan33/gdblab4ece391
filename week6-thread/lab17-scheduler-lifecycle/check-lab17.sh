#!/usr/bin/env bash
set -eu
MODE="${MODE:-solution}"; ELF="build/${MODE}/kernel.elf"
NM="${NM:-riscv64-unknown-elf-nm}"
for s in thread_created thread_setup_entry scheduler_before_switch \
 scheduler_after_select thread_function_returned thread_marked_exited \
 condition_wait_entered condition_thread_sleeping condition_broadcast_done \
 condition_thread_resumed lab17_todo_checkpoint; do
 "${NM}" "$ELF" | awk '{print $3}' | grep -Fxq "$s" ||
  { echo "Missing checkpoint: $s"; exit 1; }
done
echo "Lab 17 static checks passed."
