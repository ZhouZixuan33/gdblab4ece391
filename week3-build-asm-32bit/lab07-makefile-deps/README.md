# Lab 07: Makefile Dependencies and Stale Builds

## Goal

Use `make`, `make -n`, `make clean`, file timestamps, and Makefile dependency rules to prove when a binary does not match the source you think you built.

## Failure Scenario

Run the program:

```bash
make run
```

You should see output similar to:

```text
Scheduler configuration
MAX_TASKS compiled into this binary: 4
SCHEDULER_QUANTUM_MS: 50
Requested tasks: 6
Admitted tasks: 4
If you changed config.h but this output did not change, suspect a stale object file.
```

Now edit `config.h` and change:

```c
#define MAX_TASKS 4
```

to:

```c
#define MAX_TASKS 6
```

Run:

```bash
make run
```

The output may still say `MAX_TASKS compiled into this binary: 4`. The bug is not in GDB and not in the C expression. The object file did not rebuild because the Makefile says `build/main.o` depends on `main.c`, but it does not say that `build/main.o` also depends on `config.h`.

## Where This Shows Up / Common Scenarios

This pattern appears when a header changes but the binary still behaves like the old source:

- a syscall number changes
- a struct layout changes
- a constant in a shared header changes
- a table size changes
- a function prototype changes

In ECE391-style work, stale object files can make GDB look confusing because GDB is honestly debugging the executable you gave it, even if that executable is not the one you thought you rebuilt.

## Concept Warmup

This lab uses a build-debugging loop:

```text
observe stale behavior -> ask Make what it would do -> inspect dependencies -> clean as a temporary reset -> fix the rule
```

### What a Basic Makefile Contains / 基本 Makefile 由什么组成

A small Makefile usually has four pieces:

```text
variables       names for tools, flags, directories, and files
targets         things Make can build or run
dependencies    files or targets that must be ready first
recipes         shell commands that actually do the work
```

Example shape:

```makefile
target: dependency1 dependency2
	command
```

Important detail: recipe lines must start with a real tab character, not spaces.

### How Make Decides What to Rebuild / Make 如何决定是否重建

Make compares timestamps:

```text
if dependency is newer than target -> rebuild target
if target does not exist          -> build target
if target is newer than inputs    -> skip it
```

That is why dependency lists matter. If a file is not listed as a dependency, Make will not know that changing that file should trigger a rebuild.

### Variables / 变量

This lab uses variables to avoid repeating paths and flags:

```makefile
CC := gcc
CFLAGS := -g -O0 -Wall -Wextra -std=c11
BUILD_DIR := build
TARGET := $(BUILD_DIR)/lab07
OBJ := $(BUILD_DIR)/main.o
```

`:=` assigns the value immediately. For beginner Makefiles, you can read it as "set this name to this value."

`$(NAME)` expands a variable:

```makefile
$(BUILD_DIR)/lab07
```

becomes:

```text
build/lab07
```

### Target / 目标

A Make target is something Make knows how to build or run. Examples in this lab:

```makefile
all
run
clean
build/lab07
build/main.o
```

The first target in a Makefile is the default target. In this lab:

```makefile
all: $(TARGET)
```

means running plain `make` is the same as asking Make to build `all`, which then requires `build/lab07`.

### Dependency / 依赖

A dependency is an input that a target depends on. If the dependency is newer than the target, Make should rebuild the target.

This rule says `build/main.o` depends only on `main.c`:

```makefile
$(OBJ): main.c | $(BUILD_DIR)
```

That is the intentional lab bug. Since `main.c` includes `config.h`, the object file also depends on `config.h`.

### Recipe / 命令配方

A recipe is the shell command under a target:

```makefile
$(OBJ): main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c main.c -o $(OBJ)
```

This compiles `main.c` into `build/main.o`.

Expanded mentally, it means:

```bash
gcc -g -O0 -Wall -Wextra -std=c11 -c main.c -o build/main.o
```

The `-c` flag means "compile only; do not link yet."

### Order-Only Dependency / 只要求顺序的依赖

This rule has two kinds of dependencies:

```makefile
$(OBJ): main.c | $(BUILD_DIR)
```

Read it as:

```text
build/main.o needs main.c as a real input.
build/main.o also needs the build directory to exist first.
```

The `| $(BUILD_DIR)` part is an order-only dependency. It tells Make to create `build/` before compiling, but a timestamp change on the `build/` directory should not force every object file to rebuild.

### Phony Targets / 伪目标

```makefile
.PHONY: all run clean
```

`all`, `run`, and `clean` are command names, not files we want to create. `.PHONY` tells Make to always treat them as actions.

Without `.PHONY`, a file named `clean` could confuse `make clean`.

### Line-by-Line Makefile Walkthrough / 逐行解释

Here is the lab Makefile with each part explained.

```makefile
CC := gcc
```

Use `gcc` as the C compiler.

```makefile
CFLAGS := -g -O0 -Wall -Wextra -std=c11
```

Use debug-friendly compiler flags:

- `-g`: include GDB debug information.
- `-O0`: disable optimization so source lines match execution more closely.
- `-Wall -Wextra`: enable useful warnings.
- `-std=c11`: compile as C11.

```makefile
BUILD_DIR := build
```

Put build outputs in the `build/` directory.

```makefile
TARGET := $(BUILD_DIR)/lab07
```

Name the final executable `build/lab07`.

```makefile
OBJ := $(BUILD_DIR)/main.o
```

Name the object file `build/main.o`.

```makefile
.PHONY: all run clean
```

Tell Make that `all`, `run`, and `clean` are action names, not real files.

```makefile
all: $(TARGET)
```

The default build asks for the final executable.

```makefile
$(TARGET): $(OBJ) | $(BUILD_DIR)
```

To build `build/lab07`, Make first needs `build/main.o`. It also needs the `build/` directory to exist.

```makefile
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)
```

Link the object file into the final executable.

Expanded:

```bash
gcc -g -O0 -Wall -Wextra -std=c11 build/main.o -o build/lab07
```

```makefile
# Intentional lab bug: config.h is missing from this dependency list.
```

This comment names the bug. `main.c` includes `config.h`, but the rule below does not list `config.h`.

```makefile
$(OBJ): main.c | $(BUILD_DIR)
```

To build `build/main.o`, Make watches `main.c` and makes sure `build/` exists.

The missing dependency is `config.h`.

```makefile
	$(CC) $(CFLAGS) -c main.c -o $(OBJ)
```

Compile `main.c` into `build/main.o`.

Expanded:

```bash
gcc -g -O0 -Wall -Wextra -std=c11 -c main.c -o build/main.o
```

```makefile
$(BUILD_DIR):
```

Define how to create the build directory.

```makefile
	mkdir -p $(BUILD_DIR)
```

Create `build/`. The `-p` flag means it is okay if the directory already exists.

```makefile
run: $(TARGET)
```

Before running, make sure the executable exists.

```makefile
	./$(TARGET)
```

Run `./build/lab07`.

```makefile
clean:
```

Define the cleanup action.

```makefile
	rm -rf $(BUILD_DIR)
```

Remove the build directory and all generated files inside it.

### Automatic Variables / 自动变量

Make has shortcuts called automatic variables. For example, many Makefiles use symbols that mean "the current target," "the first dependency," or "all dependencies."

They are useful in larger Makefiles, but this lab intentionally avoids them. The explicit form is easier to read first:

```makefile
$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)
$(CC) $(CFLAGS) -c main.c -o $(OBJ)
```

instead of a shorter but less beginner-friendly form.

For ECE391 work, you should be able to recognize automatic variables when reading a provided or existing Makefile, but you do not need them to write a basic Makefile. For MP-style assignments, the safest habit is:

```text
first understand the given Makefile
then make the smallest needed change
use explicit rules when clarity matters
use automatic variables only when the Makefile pattern is already using them
```

Course materials can change by semester, so always follow your current assignment handout and provided Makefile. The key requirement is not memorizing shortcuts; it is understanding which source files, headers, object files, and final binaries depend on each other.

### Stale Object File / 过期目标文件

An object file is stale when it was built from older inputs. In this lab, `build/main.o` can be stale because `config.h` changed but Make did not rebuild `main.o`.

Temporary reset:

```bash
make clean
make
```

Real fix:

```makefile
$(OBJ): main.c config.h | $(BUILD_DIR)
```

## Guided Mode

Step 1: Build and run the program.

```bash
make
make run
```

What to look for / 看什么: the binary prints the `MAX_TASKS` value compiled into it.

Step 2: Change `config.h`.

Change `MAX_TASKS` from `4` to `6`, then run:

```bash
make run
```

What to look for / 看什么: if the output still says `4`, the executable is stale.

Step 3: Ask Make what it plans to do.

```bash
make -n
```

Meaning / 是什么: print the commands Make would run without actually running them.

When to use / 什么时候用: use this when you expected a rebuild but Make says "nothing to be done" or silently runs the old binary.

What to look for / 看什么: Make does not compile `main.c` again after only `config.h` changed.

Step 4: Inspect timestamps.

```bash
ls -l config.h main.c build/main.o build/lab07
```

Why this helps / 为什么有用: if `config.h` is newer than `build/main.o`, but Make still skips rebuilding `build/main.o`, the dependency graph is incomplete.

Step 5: Use `make clean` as a temporary reset.

```bash
make clean
make run
```

What to look for / 看什么: the output now reflects the new header value because all build products were removed and rebuilt.

Step 6: Fix the real dependency.

Change the object rule in `Makefile` from:

```makefile
$(OBJ): main.c | $(BUILD_DIR)
```

to:

```makefile
$(OBJ): main.c config.h | $(BUILD_DIR)
```

Step 7: Confirm the fix.

Change `MAX_TASKS` again, then run:

```bash
make -n
make run
```

What to look for / 看什么: Make should compile `main.c` before linking/running.

## Hint Mode

1. Run the program and note the compiled `MAX_TASKS` value.
2. Change `MAX_TASKS` in `config.h`.
3. Run `make -n`.
4. Compare the Makefile dependency rule for `build/main.o` with the files included by `main.c`.
5. Use `make clean` to prove the C code itself is not the problem.
6. Add the missing header dependency.

## Review Questions

1. What command shows what Make would do without running the commands?

   Answer:

   ```bash
   make -n
   ```

2. Why can `make clean` appear to fix this lab?

   Answer: it removes the stale object file, forcing Make to rebuild everything from the current source and header files.

3. Why is `make clean` not the real fix?

   Answer: the dependency rule is still incomplete. The next header-only change can create another stale object file.

4. What file is missing from the `build/main.o` dependency rule?

   Answer: `config.h`.

5. What is the corrected rule?

   Answer:

   ```makefile
   $(OBJ): main.c config.h | $(BUILD_DIR)
   ```

6. Why does this matter for GDB?

   Answer: GDB debugs the executable you give it. If the executable is stale, GDB may show behavior that does not match your current source.
