# Week 4 and Week 5 QEMU, Remote GDB, and ECE391 Debugging Design

Date: 2026-07-08

## Purpose

Week 4 and Week 5 will complete the debugging curriculum by moving from local user-space and 32-bit assembly practice into QEMU remote debugging and ECE391-style kernel debugging.

Week 4 will stay as a sequence of four focused labs. It teaches the core mechanics one step at a time: starting a QEMU target, attaching GDB remotely, loading symbols, setting breakpoints by name and address, reading register state, recognizing exception-style failures, and using QEMU logs when the target resets.

Week 5 will be a comprehensive capstone lab. It will combine the Week 4 mechanics into a mini-kernel debugging workflow that feels close to real ECE391 preparation while still remaining small, reproducible, and beginner-friendly.

The main learning goal is not to write an operating system. The goal is to make a learner comfortable enough with boot, QEMU, GDB, symbols, addresses, registers, stack state, and failure triage that the first real ECE391 debugging session feels familiar instead of overwhelming.

## Target Learner

The learner has completed the earlier labs and can already:

- Use GDB breakpoints, stepping, printing, backtraces, frames, watchpoints, and core files.
- Inspect memory, registers, and disassembly in local programs.
- Use `make`, understand stale builds, inspect symbols, and debug simple 32-bit x86 C/assembly boundaries.

The learner is not yet comfortable with:

- Running a target under QEMU instead of as a normal Linux process.
- Connecting GDB with `target remote`.
- Loading symbols for a remote target.
- Understanding why a breakpoint by symbol may fail or why an address breakpoint may be needed.
- Distinguishing a hang, wrong entry point, bad stack, invalid pointer, and reset-like failure.
- Using QEMU logs to investigate exception and CPU reset behavior.

## Design Principles

Week 4 and Week 5 should follow the existing repository style:

- One lab directory per focused exercise, except Week 5 which intentionally contains one larger capstone lab.
- `README.md` remains the primary teaching artifact.
- Small, reproducible target programs with concrete failure symptoms.
- `Makefile` targets for normal build, run, debug, GDB attach commands, and cleanup.
- Verification scripts that build artifacts and check required tools without requiring interactive GDB sessions.
- Guided Mode, Hint Mode, and Review Questions in every lab.
- Chinese-English terminology where new QEMU, boot, remote debugging, and exception concepts appear.

The labs should teach a practical triage habit:

```text
Is the target running, halted, hung, faulting, resetting, or simply missing symbols?
```

The learner should learn to answer that question with QEMU, GDB, symbols, registers, memory, disassembly, and logs.

## Week 4 Overview

Week 4 will keep the existing planned structure:

```text
week4-qemu-remote-gdb/
  README.md
  lab11-qemu-hello/
  lab12-remote-breakpoints/
  lab13-registers-and-exceptions/
  lab14-mini-kernel-debug/
```

Week 4 leans toward ECE391-style debugging, but each lab stays small. It should not become the capstone. Its job is to isolate and teach the parts that Week 5 will combine.

## Week 4 Lab Sequence

### Lab 11: QEMU Hello Target

Goal: Teach the minimum remote-debug loop: build a tiny bootable target, start QEMU paused, connect GDB, inspect initial CPU state, and continue execution.

Failure scenario: The target appears to do nothing because QEMU is intentionally started paused with `-S`. The learner must attach GDB and continue execution before visible progress occurs.

Planned files:

```text
week4-qemu-remote-gdb/lab11-qemu-hello/
  README.md
  Makefile
  boot.asm
  linker.ld
```

Target shape:

- A tiny 32-bit x86 boot or kernel-style target.
- Prints a short message through a simple debug channel, serial port, or QEMU-compatible text output.
- Provides enough symbols for GDB to identify the entry routine.
- Avoids paging, IDT setup, and complex device initialization.
- Uses NASM for boot assembly so the Week 4 and Week 5 boot stubs share one assembler style.

Core shell commands:

```bash
make
make run
make debug
make gdb
```

Core GDB commands:

```gdb
target remote :1234
info registers
x/i $eip
continue
detach
quit
```

Guided learning path:

1. Build the target.
2. Start QEMU normally and observe the visible success message.
3. Start QEMU in debug mode with `-s -S`.
4. Connect GDB to `:1234`.
5. Inspect `eip`, `esp`, and the current instruction.
6. Continue execution and observe the target progress.
7. Explain why remote debugging differs from `gdb ./program`.

ECE391 connection: In kernel-style work, the code under debug is often not a normal Linux process. GDB controls a remote CPU inside QEMU, so the first skill is attaching to the target and orienting yourself from registers and symbols.

### Lab 12: Remote Breakpoints, Symbols, and Raw Addresses

Goal: Teach how symbols and raw addresses work in a remote QEMU target.

Failure scenario: The target runs too quickly or produces little output. The learner must stop at known functions and at a raw instruction address to prove where execution goes.

Planned files:

```text
week4-qemu-remote-gdb/lab12-remote-breakpoints/
  README.md
  Makefile
  boot.asm
  kernel.c if useful
  linker.ld
```

Target shape:

- A small target with named routines such as `kernel_entry`, `init_console`, and `debug_checkpoint`.
- One routine prints a visible checkpoint or writes a known value.
- The link address is stable enough for raw address breakpoints to be teachable.

Core shell commands:

```bash
make
make debug
make gdb
nm build/kernel.elf
objdump -d build/kernel.elf
```

Core GDB commands:

```gdb
file build/kernel.elf
target remote :1234
symbol-file build/kernel.elf
info files
info functions
break kernel_entry
break debug_checkpoint
break *0x00100000
info breakpoints
continue
```

Guided learning path:

1. Build a symbol-bearing ELF and a bootable image if the lab uses both.
2. Inspect symbols with `nm` and disassembly with `objdump`.
3. Start QEMU paused for GDB.
4. Connect GDB and load symbols.
5. Set a breakpoint by function name.
6. Set a breakpoint by raw address.
7. Compare what GDB knows before and after `symbol-file`.
8. Explain why ECE391 debugging often needs both symbolic and address-level breakpoints.

ECE391 connection: Early kernel code may fail before output exists. A learner needs to stop by entry symbol, handler symbol, or raw address to prove whether the CPU reached the code they expected.

### Lab 13: Registers and Exception-Style Failures

Goal: Teach how to inspect CPU state when the target hits controlled failure scenarios that resemble low-level exceptions.

Failure scenarios:

- Page-fault-like invalid memory access.
- Invalid-instruction or protection-like failure.
- Bad jump to an unexpected address.

Planned files:

```text
week4-qemu-remote-gdb/lab13-registers-and-exceptions/
  README.md
  Makefile
  boot.asm
  kernel.c or failure_cases.c
  linker.ld
```

Target shape:

- A tiny target with selectable failure modes, either by Make variable or separate build targets.
- The failures should be deliberate, deterministic, and easy to reset.
- The lab may use QEMU logs to show exception or reset clues, but should not require a full IDT implementation before the learner can practice the workflow.

Core shell commands:

```bash
make SCENARIO=bad-pointer
make SCENARIO=bad-jump
make SCENARIO=invalid-instruction
make debug
qemu-system-i386 ... -d int,cpu_reset -D qemu.log
```

Core GDB commands:

```gdb
target remote :1234
info registers
x/i $eip
x/10i $eip
x/32xw $esp
disassemble
bt
```

Guided learning path:

1. Run a known-good scenario to confirm the environment.
2. Run the bad pointer scenario and stop near the failure.
3. Inspect `eip`, `esp`, and the faulting instruction.
4. Run the bad jump scenario and identify the unexpected control-flow target.
5. Run the invalid-instruction or protection-like scenario and compare it with a pointer-style failure.
6. Use a QEMU log when the target resets too quickly to inspect interactively.
7. Build a habit of recording the first bad instruction, not only the final symptom.

ECE391 connection: Kernel bugs often show up as CPU state rather than a friendly C backtrace. The learner needs to start with registers, instruction pointer, stack pointer, disassembly, and QEMU logs.

### Lab 14: Mini-Kernel Debug Triage

Goal: Combine the Week 4 tools in a small multi-scenario lab before the larger Week 5 capstone.

Failure scenarios:

- Early boot hang: the target stops making visible progress.
- Wrong entry address: execution starts or jumps somewhere unexpected.
- Reset-like behavior: QEMU appears to restart, go black, or return to startup code.

Planned files:

```text
week4-qemu-remote-gdb/lab14-mini-kernel-debug/
  README.md
  Makefile
  boot.asm
  kernel.c
  linker.ld
```

Target shape:

- A small kernel-style target with two or three selectable broken modes.
- The target should be simple enough to understand in one sitting.
- It should reuse the command patterns from Labs 11-13.

Core commands:

```bash
make SCENARIO=hang debug
make SCENARIO=wrong-entry debug
make SCENARIO=reset debug
make gdb
```

```gdb
target remote :1234
symbol-file build/kernel.elf
break kernel_entry
break *address
continue
Ctrl-C
info registers
x/i $eip
x/32xw $esp
```

Guided learning path:

1. Start from the visible symptom.
2. Decide whether to interrupt, set a breakpoint, inspect symbols, or enable QEMU logs.
3. Identify where execution is currently stopped.
4. Compare the current `eip` with expected entry or checkpoint symbols.
5. Inspect stack state when control flow looks suspicious.
6. Explain the minimum fix conceptually.

ECE391 connection: This lab teaches symptom-based triage. A learner should leave Week 4 knowing which first commands to run when the kernel does not visibly progress.

## Week 5 Overview

Week 5 will be the comprehensive ECE391-preparation lab.

```text
week5-mini-kernel-capstone/
  README.md
  lab15-mini-kernel-debug-capstone/
```

Suggested capstone structure:

```text
week5-mini-kernel-capstone/lab15-mini-kernel-debug-capstone/
  README.md
  Makefile
  boot.asm
  kernel.c
  console.c
  console.h
  debug_scenarios.c
  debug_scenarios.h
  linker.ld
  gdbinit.example
```

Week 5 should feel more like a real debugging session than the Week 4 labs. It can be longer, with fewer hints at the start and stronger guided recovery sections afterward.

## Week 5 Lab 15: Mini-Kernel Debug Capstone

Goal: Give the learner a realistic, repeatable mini-kernel debugging environment that prepares them for ECE391 boot and early-kernel debugging.

The capstone should teach a complete workflow:

```text
verify environment -> build known-good target -> run under QEMU -> attach GDB
-> load symbols -> choose triage path -> inspect CPU state -> identify root cause
```

### Scenario Set

The capstone should include scenario selection through a Make variable:

```bash
make SCENARIO=good
make SCENARIO=hang
make SCENARIO=wrong-entry
make SCENARIO=bad-stack
make SCENARIO=bad-pointer
make SCENARIO=reset
```

Each scenario should be deterministic.

`SCENARIO=good`:

- Confirms that QEMU, GDB, image generation, symbols, and expected output work.
- Used as the learner's baseline before debugging broken scenarios.

`SCENARIO=hang`:

- The target loops during early startup after reaching a known checkpoint.
- Teaches interrupting or halting the target, then inspecting `eip` and disassembly.

`SCENARIO=wrong-entry`:

- The entry symbol, link address, or jump target does not match expectation.
- Teaches `info files`, `nm`, `objdump`, `symbol-file`, and raw address breakpoints.

`SCENARIO=bad-stack`:

- The stack pointer is missing, wrong, or corrupted before a call or return.
- Teaches `info registers`, `x/32xw $esp`, return-address inspection, and stack plausibility.

`SCENARIO=bad-pointer`:

- Kernel-style invalid pointer or NULL-like access.
- Teaches faulting instruction analysis, address inspection, and source-to-disassembly mapping.

`SCENARIO=reset`:

- A reset or triple-fault-like failure occurs before normal output completes.
- Teaches QEMU logging with `-d int,cpu_reset -D qemu.log` and how to extract the first useful clue.

### Required Make Targets

The capstone should make the environment friendly for beginners:

```bash
make
make run
make debug
make gdb
make log
make clean
make check-tools
```

Recommended behavior:

- `make run`: run the selected scenario normally.
- `make debug`: start QEMU with `-s -S` and wait for GDB.
- `make gdb`: start GDB with the right executable and a suggested command list.
- `make log`: run QEMU with exception and CPU reset logging.
- `make check-tools`: verify `qemu-system-i386`, `gdb` or `gdb-multiarch`, `nasm` or assembler, `ld`, `objdump`, and `nm`.

The Makefile should print concise next-step hints after `make debug`, such as:

```text
In another terminal:
  make gdb
Then:
  target remote :1234
  symbol-file build/kernel.elf
```

### Required Documentation Sections

The Lab 15 README should include:

- Goal.
- Environment check.
- Known-good baseline.
- Failure scenarios.
- QEMU/GDB mental model.
- Guided Mode for each scenario.
- Hint Mode that asks the learner to choose a triage path.
- Review questions.
- ECE391 transfer notes.

The capstone should also include a compact startup checklist:

```text
1. Can the good scenario build?
2. Can QEMU run it?
3. Can QEMU start paused?
4. Can GDB connect?
5. Are symbols loaded?
6. Does eip match expected code?
7. Is esp plausible?
8. Is the target hung, faulting, or resetting?
```

## Supporting Repository Updates

### Root README

Update the learning path:

```text
Week 4: QEMU and GDB remote debugging.
Week 5: mini-kernel debugging capstone for ECE391 preparation.
```

Add verification commands:

```bash
bash scripts/verify-week4.sh
bash scripts/verify-week5.sh
```

### Week Overviews

Update `week4-qemu-remote-gdb/README.md` with:

- Week 4 purpose.
- Lab list.
- Required packages.
- First-response QEMU/GDB habits.
- Warning that these labs use QEMU and may require two terminals.

Create `week5-mini-kernel-capstone/README.md` with:

- Week 5 purpose.
- Capstone description.
- Environment checklist.
- Scenario list.
- Recommended workflow.

### Setup Docs

Ensure setup docs include:

```bash
sudo apt install qemu-system-x86 gdb-multiarch nasm binutils make
```

Also mention that learners may use plain `gdb` if it works for the local target, but `gdb-multiarch` is a safer recommendation for remote target workflows.

### Command Cheatsheet

Add or expand symptom recipes:

```text
QEMU target waits forever:
  target remote :1234
  info registers
  x/i $eip
  continue

Remote breakpoint by function does not work:
  symbol-file build/kernel.elf
  info files
  info functions
  nm build/kernel.elf

Execution starts at a strange address:
  info registers
  p/x $eip
  info files
  objdump -d build/kernel.elf
  break *addr

Stack looks wrong:
  info registers
  p/x $esp
  x/32xw $esp

QEMU resets before GDB can inspect:
  qemu-system-i386 ... -d int,cpu_reset -D qemu.log
```

### Terminology

Add or strengthen terms:

- `remote target / remote debug target`
- `QEMU`
- `symbol file`
- `entry point`
- `link address`
- `boot image`
- `CPU exception`
- `double fault`
- `triple fault`
- `QEMU log`

Each term should include:

- Meaning.
- How it appears in commands or GDB.
- Debugging use.
- ECE391 connection.

### Verification Scripts

Add:

```text
scripts/verify-week4.sh
scripts/verify-week5.sh
```

They should:

- Check for required tools with readable messages.
- Build each lab target.
- Avoid starting long-running interactive QEMU sessions.
- Optionally run quick non-interactive QEMU smoke tests only if they exit reliably.
- Print setup reminders when QEMU, `gdb-multiarch`, or `nasm` is missing.

Suggested missing-tool message:

```text
Missing QEMU/GDB remote debugging tools.
Install:
  sudo apt install qemu-system-x86 gdb-multiarch nasm
```

## Error Handling and Environment Constraints

QEMU labs have more environment friction than earlier weeks. The design should assume beginners may hit setup problems before they hit debugging problems.

Requirements:

- Every QEMU lab must distinguish environment failure from target failure.
- Build failures should preserve compiler, assembler, or linker output.
- `make check-tools` or verification scripts should report missing tools directly.
- Lab READMEs should tell learners when a command needs a second terminal.
- If QEMU display behavior differs across environments, labs should prefer serial output or `-nographic` where practical.
- If port `1234` is already in use, the README should explain how to stop the old QEMU process or choose a different GDB port.

Beginner-friendly troubleshooting notes should cover:

```text
Connection timed out:
  Is QEMU running with -s -S?
  Is another process already using port 1234?

Breakpoint by name fails:
  Did GDB load build/kernel.elf as the symbol file?
  Does nm show the symbol?

QEMU appears frozen:
  Did you start with -S and forget continue?
  Is the target intentionally in a hang scenario?

No useful output:
  Try the good scenario first.
  Use GDB to inspect eip and QEMU logs for reset behavior.
```

## Testing Strategy

Implementation should be checked at four levels:

1. Tool checks:

   ```bash
   bash scripts/verify-week4.sh
   bash scripts/verify-week5.sh
   ```

2. Per-lab build checks:

   ```bash
   make -C week4-qemu-remote-gdb/lab11-qemu-hello clean all
   make -C week4-qemu-remote-gdb/lab14-mini-kernel-debug clean all
   make -C week5-mini-kernel-capstone/lab15-mini-kernel-debug-capstone clean all
   ```

3. Known-good QEMU smoke checks:

   - Run `SCENARIO=good` for Lab 15.
   - Confirm the expected success message appears.
   - Keep this non-interactive where possible.

4. Manual GDB walkthrough checks:

   - Start QEMU with `make debug`.
   - Connect from GDB.
   - Load symbols.
   - Break on entry.
   - Inspect `eip`, `esp`, and current instruction.
   - Continue to visible target progress or expected failure.

Verification scripts should not require an interactive GDB session. The GDB practice remains in the README walkthroughs.

## Scope Boundaries

Week 4 and Week 5 will not:

- Recreate the full ECE391 codebase.
- Require implementing a full bootloader, paging subsystem, filesystem, scheduler, or shell.
- Require a complete IDT before teaching exception-style triage.
- Depend on nondeterministic crashes.
- Require graphical QEMU output if serial or `-nographic` output is more reliable.
- Teach kernel feature implementation as the main goal.

Week 5 may feel like a mini-kernel, but it is still a debugging capstone. Every scenario should exist to teach a debugging decision, not to add operating-system scope.

## Success Criteria

Week 4 succeeds if the learner can:

- Start QEMU normally and in paused remote-debug mode.
- Connect GDB to `:1234`.
- Load symbols for a remote target.
- Set breakpoints by function and raw address.
- Inspect `eip`, `esp`, registers, memory, and disassembly.
- Use QEMU logs for reset-like behavior.
- Explain the difference between a local process and a remote QEMU target.

Week 5 succeeds if the learner can:

- Verify the boot/QEMU/GDB environment from scratch.
- Establish a known-good baseline.
- Choose a triage path based on symptom: hang, wrong entry, bad stack, bad pointer, or reset.
- Use symbols, addresses, registers, stack memory, disassembly, and logs together.
- Record the first useful clue before jumping to a source-level guess.
- Transfer the workflow to ECE391 early boot and kernel debugging.

## Recommended Implementation Order

1. Implement `lab11-qemu-hello` first to settle the boot image, QEMU flags, GDB connection, and output strategy.
2. Implement `lab12-remote-breakpoints` to validate symbol loading and stable addresses.
3. Implement `lab13-registers-and-exceptions` to validate deterministic failure modes and QEMU logging.
4. Implement `lab14-mini-kernel-debug` as the small Week 4 integration lab.
5. Add `scripts/verify-week4.sh` and update Week 4 overview docs.
6. Implement `lab15-mini-kernel-debug-capstone` using the proven Week 4 build/debug pattern.
7. Add `scripts/verify-week5.sh` and the Week 5 overview.
8. Update root README, setup docs, command cheatsheet, terminology, and ECE391 debugging map.

This order reduces risk. The capstone should reuse working Week 4 patterns instead of inventing its environment from scratch.
