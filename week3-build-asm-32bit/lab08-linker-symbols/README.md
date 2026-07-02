# Lab 08: Linker Symbols and Function Lookup

## Goal

Use `nm`, `objdump -t`, `readelf -s`, and GDB `info functions` to connect source declarations, object files, linker errors, and debugger function lookup.

## Failure Scenario

This lab has a normal build and a deliberate broken build.

Run the broken build:

```bash
make broken-missing-object
```

You should see a linker error similar to:

```text
undefined reference to `install_keyboard_handler'
```

The source file `main.c` calls `install_keyboard_handler()`. The header declares it. The implementation exists in `handlers.c`. The broken target fails because it links only `build/main.o` and leaves out `build/handlers.o`.

The default build is fixed:

```bash
make
make run
```

## Where This Shows Up / Common Scenarios

This pattern appears when code crosses file boundaries:

- a C file calls a helper from another C file
- C calls an assembly entry point
- a syscall table references a handler
- a linker script or Makefile omits an object file
- GDB cannot find a function name you expected

In ECE391-style code, symbols are the bridge between source names, object files, final binaries, and debugger breakpoints.

## Concept Warmup

This lab uses a symbol-debugging loop:

```text
read linker error -> inspect object symbols -> find undefined and defined names -> link the right objects -> confirm GDB can see the function
```

### Symbol / 符号

A symbol is a name in an object file or executable, often a function or global variable name.

Useful commands:

```bash
nm build/main.o
nm build/handlers.o
objdump -t build/handlers.o
readelf -s build/handlers.o
```

These are normal Linux command-line tools, not GDB commands and not shell built-ins. On Ubuntu they usually come from the `binutils` package:

```bash
sudo apt install binutils
```

This repo's base setup already installs `binutils`.

`nm` prints symbols from object files and executables. It is the simplest first tool for this lab because it shows compact symbol letters such as `U` and `T`:

```bash
nm build/main.o
nm build/handlers.o
```

`objdump` can inspect many parts of object files and executables. With `-t`, it prints the symbol table:

```bash
objdump -t build/handlers.o
```

Use `objdump` when you want a broader binary-inspection tool that can also disassemble code in later labs.

`readelf` inspects ELF files, which are the normal object/executable format on Linux. With `-s`, it prints the ELF symbol table:

```bash
readelf -s build/handlers.o
```

Use `readelf` when you want to see symbols in the ELF format's own terms, including fields such as value, size, type, binding, section index, and name.

For this beginner lab, start with `nm`. Use `objdump -t` and `readelf -s` to confirm that the same symbol information exists in the object file from different tool views.

### Compile vs Link / 编译和链接

Compile and link are two different stages.

Compile turns each `.c` file into an object file:

```bash
gcc -c main.c -o build/main.o
gcc -c handlers.c -o build/handlers.o
```

At this stage, each source file is handled mostly on its own:

```text
main.c      -> build/main.o
handlers.c  -> build/handlers.o
```

An object file is like one program piece. It can say, "I use a function with this name, but the function body is somewhere else."

Link combines object files into the final executable:

```bash
gcc build/main.o build/handlers.o -o build/lab08
```

The linker's job is to connect symbol uses to symbol definitions:

```text
main.o needs install_keyboard_handler
handlers.o provides install_keyboard_handler
linker connects them
```

That is why the fixed link command must include both object files.

The broken target links only `build/main.o`:

```bash
gcc build/main.o -o build/lab08-missing-object
```

That fails because `main.o` uses `install_keyboard_handler`, but the linker was not given `handlers.o`, which contains the function body.

Memory hook / 记忆钩子:

```text
compile: make pieces
link: connect pieces
```

### Undefined Reference / 未定义引用

An undefined reference means one object file uses a symbol, but the linker did not receive any object file that defines it.

In `nm` output:

```text
U install_keyboard_handler
```

means this object uses the symbol but does not define it.

In `nm` output:

```text
T install_keyboard_handler
```

means this object defines the function in its text/code section.

## Guided Mode

Step 1: Trigger the broken link.

```bash
make clean
make broken-missing-object
```

What to look for / 看什么: the linker reports `undefined reference to 'install_keyboard_handler'`.

Step 2: Build the object files without linking the final program.

```bash
make objects
```

Meaning / 是什么: compile `main.c` and `handlers.c` into `.o` files so you can inspect each object separately.

Step 3: Inspect symbols with `nm`.

```bash
nm build/main.o
nm build/handlers.o
```

What to look for / 看什么:

- `build/main.o` has `U install_keyboard_handler`.
- `build/handlers.o` has `T install_keyboard_handler`.

How to read the first letter:

- `U` means undefined. `build/main.o` uses `install_keyboard_handler`, but this object file does not provide the function body.
- `T` means text section. `build/handlers.o` defines `install_keyboard_handler` as code that the linker can use.

Why this helps / 为什么有用: it proves the missing definition exists, but the broken link command did not include the object file that defines it.

Step 4: Inspect the same idea with `objdump` and `readelf`.

```bash
objdump -t build/handlers.o
readelf -s build/handlers.o
```

What to look for / 看什么: both tools show a symbol table entry for `install_keyboard_handler`.

Step 5: Build the fixed executable.

```bash
make
make run
```

What to look for / 看什么: the program prints that the keyboard handler is installed.

Step 6: Confirm GDB can find the function name.

```bash
gdb ./build/lab08
```

```gdb
info functions install
break install_keyboard_handler
run
bt
```

Why this helps / 为什么有用: once the function is linked into the executable with symbols, GDB can use the readable function name.

## Hint Mode

1. Run `make broken-missing-object`.
2. Copy the exact missing symbol name from the linker error.
3. Build object files with `make objects`.
4. Use `nm` on `build/main.o` and `build/handlers.o`.
5. Find which object uses the symbol and which object defines it.
6. Compare the broken link command with the fixed link command in the Makefile.
7. Use GDB `info functions` after the fixed build.

## Review Questions

1. What does `U install_keyboard_handler` mean in `nm` output?

   Answer: the object file uses `install_keyboard_handler`, but does not define it.

2. What does `T install_keyboard_handler` mean in `nm` output?

   Answer: the object file defines `install_keyboard_handler` in its text/code section.

3. Which object file defines `install_keyboard_handler` in this lab?

   Answer: `build/handlers.o`.

4. Why does `make broken-missing-object` fail?

   Answer: it links `build/main.o` without `build/handlers.o`, so the linker cannot find the implementation.

5. What command lists functions known to GDB?

   Answer:

   ```gdb
   info functions
   ```

6. Why do symbol tools matter before a GDB session?

   Answer: if a symbol is not present in the object file or final binary, GDB cannot set a normal function-name breakpoint on it.
