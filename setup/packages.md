# Package Notes

## Required for Week 1-2

```bash
sudo apt install build-essential gdb make binutils
```

- `gcc`: compiles C programs.
- `make`: runs build recipes.
- `gdb`: debugger.
- `binutils`: provides tools such as `objdump`, `nm`, and `readelf`.

## Required for Week 3

```bash
sudo apt install gcc-multilib libc6-dev-i386
```

- `gcc-multilib`: allows `gcc -m32` on an x86_64 Ubuntu VM.
- `libc6-dev-i386`: provides 32-bit C library headers and startup files needed by many `-m32` builds.
- `binutils`: already installed for Week 1-2, and provides `objdump`, `nm`, and `readelf`.

## Required for Week 4

```bash
sudo apt install qemu-system-misc gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf gdb-multiarch make
```

- `qemu-system-riscv64`: runs the RISC-V `virt` virtual machine target.
- `gcc-riscv64-unknown-elf`: builds freestanding RISC-V `.S` and C targets.
- `binutils-riscv64-unknown-elf`: provides RISC-V `objcopy`, `objdump`, `nm`, and `readelf`.
- `gdb-multiarch`: useful for remote debugging targets with architecture differences.
- `make`: runs the lab build, run, debug, and cleanup targets.

Week 4 can sometimes use plain `gdb`, but `gdb-multiarch` is recommended because the target is RISC-V even when the host VM is x86_64.
