#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

failed=0

check_tool() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing tool: $1"
    failed=1
  fi
}

echo "== Week 4 tool check"
check_tool riscv64-unknown-elf-gcc
check_tool riscv64-unknown-elf-objcopy
check_tool riscv64-unknown-elf-objdump
check_tool riscv64-unknown-elf-nm
check_tool riscv64-unknown-elf-readelf
check_tool qemu-system-riscv64

if ! command -v gdb-multiarch >/dev/null 2>&1 && ! command -v gdb >/dev/null 2>&1; then
  echo "Missing tool: gdb-multiarch or gdb"
  failed=1
fi

if [ "${failed}" -ne 0 ]; then
  echo
  echo "Install Week 4 tools with:"
  echo "  sudo apt install qemu-system-misc gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf gdb-multiarch make"
  echo
  exit 1
fi

build_lab() {
  local lab="$1"
  echo "== ${lab}"
  if ! make -C "${ROOT_DIR}/${lab}" clean all; then
    echo "Build failed for ${lab}."
    failed=1
  fi
  echo
}

build_scenario_lab() {
  local lab="$1"
  shift

  echo "== ${lab}"
  make -C "${ROOT_DIR}/${lab}" clean >/dev/null 2>&1

  local scenario
  for scenario in "$@"; do
    echo "-- SCENARIO=${scenario}"
    if ! make -C "${ROOT_DIR}/${lab}" SCENARIO="${scenario}" all; then
      echo "Build failed for ${lab} with SCENARIO=${scenario}."
      failed=1
    fi
  done
  echo
}

build_lab "week4-qemu-remote-gdb/lab11-qemu-hello"
build_lab "week4-qemu-remote-gdb/lab12-remote-breakpoints"
build_scenario_lab "week4-qemu-remote-gdb/lab13-registers-and-exceptions" \
  good bad-pointer bad-jump illegal-instruction
build_scenario_lab "week4-qemu-remote-gdb/lab14-mini-kernel-debug" \
  good hang wrong-entry reset

if [ "${failed}" -ne 0 ]; then
  echo "Week 4 build check failed."
  exit 1
fi

echo "Week 4 build check complete."
echo "Note: this script builds QEMU targets but does not run interactive GDB sessions."
