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
| Need to stop in QEMU code | `symbol-file`, `break`, `break *addr` | Lab 12 |
| Exception-like failure | `info registers`, `x/i $eip`, `x/32xw $esp` | Lab 13 |
| Early boot reset or hang | QEMU `-d int,cpu_reset`, GDB remote | Lab 14 |
