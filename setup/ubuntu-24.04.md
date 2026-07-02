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
sudo apt install qemu-system-x86 nasm gcc-multilib libc6-dev-i386 gdb-multiarch
```

Verify:

```bash
gcc --version
gdb --version
make --version
qemu-system-i386 --version
```

For Week 3 32-bit builds, verify:

```bash
echo 'int main(void){return 0;}' > /tmp/t.c
gcc -m32 -g -O0 /tmp/t.c -o /tmp/t32
file /tmp/t32
```

Expected: the `file` output should mention `ELF 32-bit`.
