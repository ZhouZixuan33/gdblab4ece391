#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

labs=(
  "week3-build-asm-32bit/lab07-makefile-deps"
  "week3-build-asm-32bit/lab08-linker-symbols"
  "week3-build-asm-32bit/lab09-riscv-calling-convention"
  "week3-build-asm-32bit/lab10-c-and-asm"
)

failed=0

for lab in "${labs[@]}"; do
  echo "== ${lab}"
  if ! make -C "${ROOT_DIR}/${lab}" clean all; then
    echo
    echo "Build failed for ${lab}."
    case "${lab}" in
      *lab09-*|*lab10-*)
        echo "If the RISC-V build tools are missing, install:"
        echo "  sudo apt install gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf make"
        ;;
    esac
    failed=1
  fi
  echo
done

echo "== week3-build-asm-32bit/lab08-linker-symbols broken target"
if make -C "${ROOT_DIR}/week3-build-asm-32bit/lab08-linker-symbols" clean broken-missing-object; then
  echo "Expected broken-missing-object to fail, but it succeeded."
  failed=1
else
  echo "broken-missing-object failed as expected."
fi
echo

if [ "${failed}" -ne 0 ]; then
  echo "Week 3 build check failed."
  exit 1
fi

echo "Build check complete."
echo "Note: lab07 intentionally starts with a missing header dependency."
echo "Note: lab08 includes a broken target for linker-symbol practice."
echo "Note: lab09 and lab10 build RV32 object/disassembly artifacts."
