#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

labs=(
  "week1-gdb-basics/lab01-breakpoints"
  "week1-gdb-basics/lab02-stack-backtrace"
  "week1-gdb-basics/lab03-watchpoints"
)

for lab in "${labs[@]}"; do
  echo "== ${lab}"
  make -C "${ROOT_DIR}/${lab}" clean all
  echo
done

echo "Build check complete."
echo "Note: lab02 intentionally crashes when run without arguments; debug it with GDB."
