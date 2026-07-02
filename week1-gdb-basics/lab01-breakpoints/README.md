# Lab 01: Breakpoints and Stepping

## Goal

Learn how to stop a program, step through function calls, and inspect variables when the output is wrong but the program does not crash.

## Failure Scenario

The program prints the wrong average score:

```bash
make run
```

You should see an average that is too high:

```text
Average score: 116.67
Expected: 87.50
```

The score data is correct, and `sum_scores` returns the right total. The bug is in `average_score`: it divides by `count - 1` instead of `count`.

## Concept Warmup

This lab uses a small loop of debugger actions:

```text
stop somewhere -> check where you are -> move one step -> inspect values
```

### 1. Stop Somewhere / 停在哪里

`breakpoint / 断点`: a place where GDB pauses execution.

You can set more than one breakpoint. GDB gives each breakpoint a number so you can inspect, disable, enable, or delete it later.

Common breakpoint forms:

```gdb
break main                 # stop when function main starts
break average_score        # stop when function average_score starts
break 21                   # stop at line 21 in the current source file
break main.c:21            # stop at line 21 in main.c
break *0x401176            # stop at an exact instruction address
```

Managing breakpoints:

```gdb
info breakpoints           # list all breakpoints and their numbers
disable 3                  # temporarily turn breakpoint 3 off
enable 3                   # turn breakpoint 3 back on
delete 3                   # remove breakpoint 3
delete                     # remove all breakpoints, after confirmation
```

### 2. Check Where You Are / 看当前停在哪里

`frame / 当前栈帧`: show the current function, source file, and line where GDB is stopped.

`list / 查看源码`: show source code around the current location. The short form is `l`.

Useful location commands:

```gdb
frame               # show current function and source line
list                # show source around the current line
where               # show the call stack; similar to bt
info line           # show the machine-code address range for the current source line
```

When GDB stops, it often already prints the current location:

```text
Breakpoint 2, average_score (...) at main.c:14
14	    int total = sum_scores(scores, count);
```

This means execution is stopped at `main.c`, line `14`.

### 3. Move Through Code / 控制程序往前走

`next / 越过函数`: execute the next source line but treat function calls as one step.

`step / 进入函数`: execute the next source line and enter a function call if there is one.

Use `next` when the current function is your focus. Use `step` when the helper function might contain the bug.

```gdb
next                # run the next source line, do not enter function calls
step                # run the next source line, enter function calls
continue            # keep running until the next breakpoint or program end
finish              # run until the current function returns
```

### 4. Inspect Values / 查看变量和值

`print / 打印`: show the value of a variable or expression. The short form is `p`.

Common printing commands for this lab:

```gdb
p count             # print one variable
p total             # print one local variable inside average_score
p scores[0]         # print one array element
display count       # automatically print count every time GDB stops
info locals         # print all local variables in the current stack frame
```

Useful `display` examples:

```gdb
display count             # show count every time GDB stops
display total             # show total every time GDB stops, after total is in scope
display scores[0]         # show one array element
display *scores@4         # show 4 integers starting at scores
display (double)total / count
display/x count           # show count in hexadecimal
display/i $pc             # show the current instruction
```

Managing automatic displays:

```gdb
info display              # list active displays and their numbers
undisplay 1               # remove display number 1
disable display 1         # temporarily turn display 1 off
enable display 1          # turn display 1 back on
```

Use `display` when you will step several times and want the same value printed each time. Use `p` when you only need to inspect a value once.

When `scores` is a function parameter, GDB shows it as a pointer:

```gdb
p scores
```

Example output:

```text
$3 = (const int *) 0x7fffffffe150
```

To print the 4 integers starting at that pointer:

```gdb
p *scores@4         # C-style artificial array view: {80, 90, 85, 95}
x/4dw scores        # memory view: 4 decimal words starting at scores
```

## Guided Mode

Step 1: Build with debug symbols.

Command:

```bash
make
```

Meaning / 是什么: compile the program using `-g -O0`, so GDB can map machine code back to source lines.

When to use / 什么时候用: before every source-level GDB session.

What to look for / 看什么: no compiler errors, and `build/lab01` exists.

ECE391 connection / 和 ECE391 的联系: debugging the wrong binary wastes huge amounts of time; always know what you built.

Step 2: Start GDB.

Command:

```bash
gdb ./build/lab01
```

Meaning / 是什么: load the executable into GDB.

When to use / 什么时候用: when you want to run the program under debugger control.

What to look for / 看什么: GDB should show a `(gdb)` prompt.

Step 3: Stop at `main`.

Command:

```gdb
break main
run
```

Meaning / 是什么: set a breakpoint at `main`, then start the program.

When to use / 什么时候用: when you want to inspect the program from the beginning.

What to look for / 看什么: GDB should stop before the first source line inside `main`.

Memory hook / 记忆钩子: `break` means "pause here before the program runs past the clue."

Step 4: Add more breakpoints.

Command:

```gdb
break average_score
break sum_scores
info breakpoints
```

Meaning / 是什么: set two more breakpoints and list all active breakpoints.

When to use / 什么时候用: when you want to stop at several suspicious functions during one run.

What to look for / 看什么: GDB lists each breakpoint with a number, location, and enabled/disabled status.

Memory hook / 记忆钩子: multiple breakpoints are like bookmarks for suspicious places.

Step 5: Manage breakpoints.

Command:

```gdb
disable 3
enable 3
delete 3
```

Meaning / 是什么: turn a breakpoint off, turn it back on, or remove it.

When to use / 什么时候用: when a breakpoint becomes noisy or you no longer need it.

What to look for / 看什么: `info breakpoints` should show whether a breakpoint is enabled, disabled, or gone.

Step 6: Step line by line.

Command:

```gdb
frame
list
next
next
print count
```

Meaning / 是什么: confirm the current source location, run the next source line without entering helper functions, then print a variable.

When to use / 什么时候用: when the current function is your focus.

What to look for / 看什么: `count` should be `4`.

Step 7: Enter the suspicious helper.

Command:

```gdb
step
```

Meaning / 是什么: execute the next line and enter the function call.

When to use / 什么时候用: when a helper function may be producing the wrong value.

What to look for / 看什么: you should enter `average_score`.

Step 8: Inspect the calculation.

Command:

```gdb
next
p total
p count
p scores[0]
display total
display count
info locals
```

Meaning / 是什么: move through the function and inspect the values used in the formula.

What to look for / 看什么: `total` is correct, but the denominator is `count - 1`.

Step 9: Manage automatic displays.

Command:

```gdb
info display
disable display 1
enable display 1
undisplay 1
```

Meaning / 是什么: list, temporarily disable, re-enable, or remove automatic displays.

When to use / 什么时候用: when the display output becomes too noisy or you no longer need a value printed at every stop.

What to look for / 看什么: `info display` shows each display's number, enabled status, and expression.

Step 10: Inspect the whole scores array.

Command:

```gdb
p scores
p *scores@4
x/4dw scores
display *scores@4
```

Meaning / 是什么: first print the pointer value, then print the four integers it points to using two views.

When to use / 什么时候用: when GDB shows an array parameter as a pointer and you need to inspect several elements.

What to look for / 看什么: `p *scores@4` should show the C-style values, while `x/4dw scores` shows the same values as raw memory words.

Memory hook / 记忆钩子: `@4` means "treat this starting point as an array of 4 elements"; `x/4dw` means "examine 4 decimal words."

## Review Questions

1. How do you stop at the beginning of `main`?

   Answer:

   ```gdb
   break main
   run
   ```

2. How do you stop at the start of `average_score` without stepping line by line from `main`?

   Answer:

   ```gdb
   break average_score
   continue
   ```

3. How do you see all active breakpoints?

   Answer:

   ```gdb
   info breakpoints
   ```

4. How do you delete only breakpoint number `3`?

   Answer:

   ```gdb
   delete 3
   ```

5. How do you confirm the current source file and line where GDB is stopped?

   Answer:

   ```gdb
   frame
   list
   ```

6. When should you use `next` instead of `step`?

   Answer: use `next` when you want to execute the next source line without entering a function call. Use `step` when the called function might contain the bug and you want to enter it.

7. What command continues running until the next breakpoint?

   Answer:

   ```gdb
   continue
   ```

8. How do you print `count`, `total`, and the first score?

   Answer:

   ```gdb
   p count
   p total
   p scores[0]
   ```

9. How do you print all local variables in the current stack frame?

   Answer:

   ```gdb
   info locals
   ```

10. How do you automatically print `count` every time GDB stops?

    Answer:

    ```gdb
    display count
    ```

11. How do you list and remove automatic displays?

    Answer:

    ```gdb
    info display
    undisplay 1
    ```

    Replace `1` with the display number shown by `info display`.

12. If `scores` prints as a pointer, how do you print the four scores as a C-style array?

    Answer:

    ```gdb
    p *scores@4
    ```

13. How do you inspect the same four scores as raw memory words?

    Answer:

    ```gdb
    x/4dw scores
    ```

14. What is the actual bug in this lab?

    Answer: `average_score` divides by `count - 1`, but it should divide by `count`.

15. Which commands help prove that the input score data is correct?

    Answer:

    ```gdb
    p scores[0]
    p *scores@4
    x/4dw scores
    ```
