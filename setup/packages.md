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
sudo apt install qemu-system-x86 gdb-multiarch nasm
```

- `qemu-system-i386`: runs a 32-bit x86 virtual machine target.
- `gdb-multiarch`: useful for remote debugging targets with architecture differences.
- `nasm`: assembler used by small boot or kernel-style examples.
