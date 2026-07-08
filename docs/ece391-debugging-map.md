# ECE391 Debugging Map

This map connects course-style symptoms to the labs and first commands to try.

| Symptom | Start With | Related Labs |
| --- | --- | --- |
| Wrong user-space result | `break`, `run`, `next`, `print` | Lab 01 |
| Segfault in a helper function | `bt`, `frame`, `info args` | Lab 02 |
| Global state changes unexpectedly | `watch`, `continue`, `bt` | Lab 03 |
| Memory corruption | `x`, `print &var`, `info registers` | Lab 04 |
| Bad pointer lifetime | `print ptr`, `ptype`, `x`, ASan comparison | Lab 05 |
| Crash already happened | `gdb ./prog core`, `bt` | Lab 06 |
| Build did not update | `make -n`, `make clean`, `make` | Lab 07 |
| Function name cannot be used in GDB | `nm`, `readelf -s`, `info functions` | Lab 08 |
| C source is not enough | `disassemble`, `info registers`, `si`, `ni` | Lab 09 |
| C and assembly disagree | `layout asm`, `x/i $eip`, `stepi` | Lab 10 |
| QEMU target is paused | `target remote :1234`, `continue` | Lab 11 |
| Need to stop in QEMU code | `break`, `break *addr`, `info functions` | Lab 12 |
| Exception-like failure | `info registers`, `x/i $pc`, `x/32gx $sp` | Lab 13 |
| Early boot reset or hang | QEMU `-d int,cpu_reset`, GDB remote | Lab 14 |

## Week 4 QEMU Triage

| Symptom | First Question | First Commands |
| --- | --- | --- |
| QEMU shows no target output after `make debug` | Is the CPU paused by `-S`? | `target remote :1234`, `continue` |
| Function breakpoint fails | Did GDB open the correct ELF symbol file? | `gdb-multiarch build/kernel.elf`, `info functions` |
| Entry address looks wrong | What address did the linker assign? | `riscv64-unknown-elf-nm -n build/kernel.elf`, `info files`, `break *0x80000000` |
| Target hangs after a checkpoint | Where is `pc` now? | `Ctrl-C`, `info registers`, `x/i $pc` |
| Target resets or exits too quickly | What did QEMU log before reset? | `make log`, inspect `build/.../qemu.log` |
