# Week 2: Memory, Stack, Pointers, and Core Dumps

Week 2 moves from source-level stepping into memory inspection. The goal is to recognize when a pointer or array value merely looks plausible, then use GDB to inspect addresses, raw memory, registers, heap lifetime, and saved crash state.

## Labs

- `lab04-array-overflow`: inspect adjacent stack memory after an out-of-bounds array write corrupts a nearby field.
- `lab05-heap-lifetime`: debug a use-after-free with GDB, then compare the result with AddressSanitizer.
- `lab06-core-dump`: debug a crash after the program has already exited by opening the executable plus a core file.

## Week 2 First-Response Habits

For nearby memory corruption:

```gdb
p &var
x/16xw addr
bt
info registers
```

For suspicious heap pointers:

```gdb
p ptr
ptype *ptr
x/32xb ptr
```

For an existing core file:

```bash
gdb ./build/program core
```

```gdb
bt
frame 0
info args
info locals
x/i $pc
```

To compile-check the completed Week 2 labs:

```bash
bash scripts/verify-week2.sh
```
