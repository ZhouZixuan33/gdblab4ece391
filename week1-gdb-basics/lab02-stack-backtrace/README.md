# Lab 02: Backtrace and Stack Frames

## Goal

Use `bt`, `frame`, `up`, `down`, `info args`, `info locals`, and `list` to trace a crash back to the caller that introduced bad data.

## Failure Scenario

Run the program without arguments:

```bash
make run
```

You should see a crash similar to:

```text
Segmentation fault
```

`segmentation fault / 段错误` means the program tried to access a memory address that it is not allowed to access, so the operating system stopped it.

In this lab, the crash is intentional. When no command-line argument is provided, `load_user_from_args` leaves `name` as `NULL`. That `NULL` is passed through several functions and eventually reaches:

```c
strcpy(u->name, input);
```

At that point, `input == NULL`, so `strcpy` crashes while trying to read from an invalid address.

The goal is not only to find the crashing function. The real goal is to trace the bad `NULL` value backward through the call stack until you find where it should have been checked.

## Concept Warmup

This lab uses a crash-debugging loop:

```text
run until crash -> print call stack -> select a frame -> inspect args/locals -> move toward the caller
```

### 1. Understand the Crash / 理解崩溃

Common causes of segmentation faults:

- `NULL` pointer dereference / 解引用 `NULL` 指针: using `*p`, `p[i]`, or passing `p` to a function when `p == NULL`.
- uninitialized pointer / 未初始化指针: using a pointer before assigning it a valid address.
- array out-of-bounds / 数组越界: reading or writing outside the valid range of an array.
- use-after-free / 释放后继续使用: accessing memory after it has been released.
- bad library-function argument / 错误的库函数参数: passing `NULL`, an invalid address, or a too-small buffer to a function such as `strcpy`.

Why can GDB still inspect data after a crash?

When the program touches invalid memory, the operating system sends it a `SIGSEGV` signal. If the program is running under GDB, GDB catches that signal and pauses the program at the crash site instead of letting it disappear immediately.

This means the crash scene is still available for inspection: the call stack, selected stack frame, function arguments, local variables, and source-line mapping can still be inspected if the program was compiled with `-g`.

### 2. Read the Backtrace / 读取调用栈

`backtrace / 调用栈回溯`: the active function-call chain at the moment the program stopped.

```gdb
bt                  # show the call stack
where               # another name for a backtrace
```

Visual model / 图示:

```text
main()
  calls load_user_from_args()
    calls parse_user()
      calls copy_name()
        crashes inside strcpy()

GDB backtrace:
#0 strcpy()
#1 copy_name()
#2 parse_user()
#3 load_user_from_args()
#4 main()
```

Frame `#0` is closest to the crash. Larger frame numbers are callers.

### 3. Select Stack Frames / 选择栈帧

`stack frame / 栈帧`: one function call's execution record.

Useful frame commands:

```gdb
frame 1             # select frame #1 from the backtrace
up                  # move one frame toward the caller
down                # move one frame back toward the crash
frame               # show the currently selected frame
```

Use `frame 1` when you want to jump directly to a specific frame number. Use `up` and `down` when you want to walk through the call chain one level at a time.

### 4. Inspect Args and Locals / 查看参数和局部变量

`info args / 查看函数参数`: print arguments passed into the selected stack frame.

`info locals / 查看局部变量`: print local variables inside the selected stack frame.

Useful inspection commands:

```gdb
info args           # show function arguments in the selected frame
info locals         # show local variables in the selected frame
p input             # print one suspicious argument
p name              # print one suspicious local variable
```

In this lab, the important values are:

```text
copy_name(..., input)
parse_user(..., raw_name)
load_user_from_args(..., name)
```

You are looking for the earliest frame where the bad value is still `NULL`.

### 5. View Source for a Frame / 查看当前栈帧对应源码

`list / 查看源码`: show source code near the selected frame's source location.

Useful source-view commands:

```gdb
list                # show source near the selected frame
list main.c:25      # show source near line 25 in main.c
list copy_name      # show source near function copy_name
```

`list` does not recover source code from crashed program memory. Because this lab is compiled with `-g`, the executable contains debug information that maps instruction addresses back to source files, line numbers, function names, argument names, and local variable names.

If you select a different frame with `frame`, `up`, or `down`, GDB changes the current source location to that frame's saved execution point. Then `list` shows code near that selected frame.

## Guided Mode

Step 1: Build with debug symbols.

Command:

```bash
make
```

Meaning / 是什么: compile the program with `-g -O0`, so GDB can show source lines, function names, argument names, and local variable names.

When to use / 什么时候用: before every source-level GDB session.

What to look for / 看什么: no compiler errors, and `build/lab02` exists.

Step 2: Start GDB.

Command:

```bash
gdb ./build/lab02
```

Meaning / 是什么: load the executable into GDB.

What to look for / 看什么: GDB should show a `(gdb)` prompt.

Step 3: Run until the crash.

Command:

```gdb
run
```

Meaning / 是什么: start the program under GDB.

When to use / 什么时候用: when you want GDB to catch the crash and freeze the failure site.

What to look for / 看什么: the program should stop with `SIGSEGV` or `Segmentation fault`.

Step 4: Print the backtrace.

Command:

```gdb
bt
```

Meaning / 是什么: show the active call chain.

When to use / 什么时候用: immediately after a crash.

What to look for / 看什么: frame `#0` is closest to the crash; lower-level helper frames may be symptoms, while higher caller frames may reveal where the bad value came from.

Memory hook / 记忆钩子: `bt = backtrace = trace back along the call path`.

Step 5: Move to the first frame in this lab's code.

Command:

```gdb
frame 1
list
info args
```

Meaning / 是什么: select frame `#1`, show nearby source, and inspect that function's arguments.

When to use / 什么时候用: when frame `#0` is inside a library function such as `strcpy`, and you want to inspect your own caller.

What to look for / 看什么: in `copy_name`, `input` should be `NULL`.

Step 6: Move upward to the caller.

Command:

```gdb
up
list
info args
```

Meaning / 是什么: move one frame toward the caller, show source, and inspect the caller's arguments.

What to look for / 看什么: in `parse_user`, `raw_name` should still be `NULL`.

Step 7: Move upward again to find where the bad value was introduced.

Command:

```gdb
up
list
info args
info locals
```

Meaning / 是什么: continue walking toward the caller and inspect both arguments and locals.

What to look for / 看什么: in `load_user_from_args`, local variable `name` is initialized to `NULL` and remains `NULL` when `argc <= 1`.

Step 8: Move down to compare caller and callee.

Command:

```gdb
down
info args
down
info args
```

Meaning / 是什么: move back toward the crash site and compare how the same bad value is named in different frames.

What to look for / 看什么: the same `NULL` value appears as `name`, then `raw_name`, then `input`.

Step 9: Re-run with valid input.

Command:

```gdb
run ada
```

Meaning / 是什么: run the program again, this time passing a command-line argument.

What to look for / 看什么: the program should print a loaded user instead of crashing.

Step 10: Set a breakpoint before the bad call path.

Command:

```gdb
break load_user_from_args
run
info args
info locals
next
next
info locals
```

Meaning / 是什么: stop before the value is passed downward and watch how `name` is initialized.

When to use / 什么时候用: when you want to observe the bad value before it causes a crash.

What to look for / 看什么: without a command-line argument, `name` stays `NULL`.

## Review Questions

1. What does `Segmentation fault` mean?

   Answer: the program tried to access memory it is not allowed to access, so the operating system stopped it.

2. Why can GDB still inspect the program after it crashes?

   Answer: GDB catches the `SIGSEGV` signal and pauses the program at the crash site, so the call stack, frames, arguments, locals, and source-line mapping can still be inspected.

3. What command shows the call stack after a crash?

   Answer:

   ```gdb
   bt
   ```

4. In a backtrace, which frame is closest to the crash?

   Answer: frame `#0`.

5. How do you select frame `#1`?

   Answer:

   ```gdb
   frame 1
   ```

6. What is the difference between `frame 1` and `up`?

   Answer: `frame 1` jumps directly to frame `#1`; `up` moves one frame toward the caller from wherever you currently are.

7. What command moves back toward the crash site after using `up`?

   Answer:

   ```gdb
   down
   ```

8. How do you print the arguments of the currently selected frame?

   Answer:

   ```gdb
   info args
   ```

9. How do you print local variables in the currently selected frame?

   Answer:

   ```gdb
   info locals
   ```

10. How do you show source code near the selected frame?

    Answer:

    ```gdb
    list
    ```

11. Why does `list` know which source lines to show?

    Answer: the program was compiled with `-g`, so the executable contains debug information mapping instruction addresses back to source files and line numbers.

12. What is the first frame in this lab's source code after the crash?

    Answer: usually `copy_name`, selected with `frame 1`, because frame `#0` is inside `strcpy`.

13. Which argument is bad in `copy_name`?

    Answer: `input` is `NULL`.

14. Which local variable first keeps the bad value in `load_user_from_args`?

    Answer: `name` is initialized to `NULL` and remains `NULL` when `argc <= 1`.

15. What is the actual bug in this lab?

    Answer: `load_user_from_args` does not reject the missing command-line argument before passing `name` to `parse_user`.
