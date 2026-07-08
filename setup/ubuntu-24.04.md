# Ubuntu 24.04 Setup

Target environment:

- Ubuntu 24.04 LTS
- x86_64
- SSH terminal workflow

Install the base tools:

```bash
sudo apt update
sudo apt install build-essential gdb make binutils
```

Install tools used by later labs:

```bash
sudo apt install qemu-system-misc gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf gcc-multilib libc6-dev-i386 gdb-multiarch
```

Verify:

```bash
gcc --version
gdb --version
make --version
qemu-system-riscv64 --version
riscv64-unknown-elf-gcc --version
gdb-multiarch --version
objcopy --version
```

For Week 3 32-bit builds, verify:

```bash
echo 'int main(void){return 0;}' > /tmp/t.c
gcc -m32 -g -O0 /tmp/t.c -o /tmp/t32
file /tmp/t32
```

Expected: the `file` output should mention `ELF 32-bit`.

For Week 4 QEMU remote debugging, verify:

```bash
qemu-system-riscv64 --version
gdb-multiarch --version
riscv64-unknown-elf-gcc --version
riscv64-unknown-elf-objdump --version
```

If `gdb-multiarch` is unavailable but plain `gdb` is installed, the labs can usually be run with:

```bash
make GDB=gdb gdb
```
