#!/usr/bin/env bash
set -eu

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

labs=(
  "week2-memory-debugging/lab04-array-overflow"
  "week2-memory-debugging/lab05-heap-lifetime"
  "week2-memory-debugging/lab06-core-dump"
)

for lab in "${labs[@]}"; do
  echo "== ${lab}"
  make -C "${ROOT_DIR}/${lab}" clean all
  echo
done

echo "== week2-memory-debugging/lab05-heap-lifetime ASan build"
make -C "${ROOT_DIR}/week2-memory-debugging/lab05-heap-lifetime" asan-build
echo

echo "Build check complete."
echo "Note: lab05 intentionally reports heap-use-after-free with 'make asan'."
echo "Note: lab06 intentionally crashes when run; use it to generate a core file."
