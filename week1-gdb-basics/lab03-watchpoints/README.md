# Lab 03: Watchpoints

## Goal

Use `watch`, `rwatch`, `awatch`, `condition`, `bt`, `frame`, `list`, and `info locals` to stop exactly when a value changes unexpectedly and identify the code path that performed the write.

This lab has two watchpoint practices:

- Practice 1: watch a struct field, `state.ready_count`.
- Practice 2: watch a pointer expression or array element, such as `*tracked_slot` or `ready_slots[tracked_index]`.

## Failure Scenarios

### Practice 1: Struct Field Watchpoint

The scheduler starts with three ready processes:

```text
Practice 1: ready_count watchpoint
Initial ready_count: 3
```

After several scheduler ticks, `ready_count` should still be `3`, because no process actually finishes in this toy scenario:

```bash
make run
```

You should see output similar to:

```text
Practice 1: ready_count watchpoint
Initial ready_count: 3
tick=1 current_pid=11 ready_count=3
tick=2 current_pid=12 ready_count=2
tick=3 current_pid=10 ready_count=1
tick=4 current_pid=11 ready_count=0
Expected ready_count to remain 3.
```

The symptom is visible at the `printf`, but the print statement is not the bug. The bad write happened earlier, somewhere during `scheduler_tick`.

By the end of Practice 1, you should be able to prove that `cleanup_finished_processes` decrements `state.ready_count` when `state.ticks >= 2`. In this toy scenario, no process has actually finished, so this cleanup path should not change the ready count.

### Practice 2: Pointer and Array Watchpoints

The ready queue also has three slots:

```text
Practice 2: pointer and array watchpoints
Initial ready_slots: [10, 11, 12] tracked_index=1 tracked_value=11
```

Slot `1` should still contain process `11` after `rebuild_ready_queue`, but it is changed through a pointer:

```text
After rebuild_ready_queue: [10, -1, 12] tracked_index=1 tracked_value=-1
Expected ready_slots[1] to remain 11.
```

By the end of Practice 2, you should be able to prove that `mark_tracked_slot_not_ready` writes through an `int *`:

```c
*slot = -1;
```

The important point is that the same memory can be watched through different expressions:

```gdb
watch *tracked_slot
watch ready_slots[1]
watch ready_slots[tracked_index]
```

These expressions are different names for the same integer when `tracked_slot == &ready_slots[1]` and `tracked_index == 1`.

## Why This Matters for ECE391

In systems code, the symptom often appears long after the bad write. A process table, file descriptor table, paging structure, or global scheduler value may be corrupted by a helper that looked unrelated.

If you only step forward line by line, you have to guess which helper is suspicious. If you set a watchpoint on the corrupted value, GDB turns the question around:

```text
Instead of asking "where should I step next?"
ask "who changed this memory?"
```

That is why watchpoints are powerful for kernel-style bugs: they attach the debugger to the state that must stay correct, not to your current guess about which function is guilty.

## Concept Warmup

This lab uses a state-corruption debugging loop:

```text
stop before the state changes -> watch the suspicious value -> continue -> inspect the writer -> explain the bad condition
```

### 1. Watch a Value

`watchpoint`: a debugger stop condition attached to a value or memory location.

Useful watchpoint forms:

```gdb
watch state.ready_count          # stop when this struct field changes
watch *tracked_slot              # stop when the int pointed to by tracked_slot changes
watch ready_slots[1]             # stop when this exact array element changes
watch ready_slots[tracked_index] # stop when the indexed array element changes
rwatch *tracked_slot             # stop when the pointed-to int is read
awatch ready_slots[tracked_index] # stop when the indexed element is read or written
```

Use `watch` first in most debugging sessions. If a value becomes wrong, the write that changed it is usually the most important event.

`rwatch` means "read watchpoint." It stops when the expression is read, even if the value does not change. Use it when the bug is about an unexpected read, such as code checking a flag too early or reading from a buffer that should no longer be used.

`awatch` means "access watchpoint." It stops when the expression is either read or written. Use it when you do not yet know whether the suspicious access is a read or a write. It is noisier than `watch`, so switch back to `watch` once you know you are chasing a bad write.

In this lab, Practice 1 and Practice 2 mostly use `watch` because both failures are corruptions: a value changes from the expected value to a wrong value.

When GDB stops on a watchpoint, it normally prints the old value and the new value:

```text
Hardware watchpoint 2: state.ready_count

Old value = 3
New value = 2
cleanup_finished_processes () at main.c:43
```

For Practice 2, the same idea applies:

```text
Hardware watchpoint 3: *tracked_slot

Old value = 11
New value = -1
mark_tracked_slot_not_ready (...) at main.c:73
```

This is the key clue. The stop location is not where the wrong value was later printed. It is the line that just changed the watched memory.

### 2. Why Watchpoints Work

A normal breakpoint is attached to code:

```text
pause when execution reaches this function or line
```

A watchpoint is attached to data:

```text
pause when this expression's value changes
```

That difference matters when you know the value that becomes wrong but do not know which code path is responsible.

For `watch state.ready_count`, GDB evaluates the struct-field expression and watches the memory for that field.

For `watch *tracked_slot`, GDB evaluates `tracked_slot`, follows the pointer, and watches the integer stored at the pointed-to address.

For `watch ready_slots[tracked_index]`, GDB evaluates the index expression and watches that array element. In this lab, that means `ready_slots[1]`.

Memory hook: a breakpoint follows control flow; a watchpoint follows data flow.

### 3. Inspect the Writer

When a watchpoint fires, the current frame is the function that performed the write, or the closest source-level function GDB can show.

Useful commands after a watchpoint fires:

```gdb
bt                  # show how execution reached this write
frame               # show the current frame
list                # show source around the write
info locals         # show local variables in the current frame
p state             # print the whole global scheduler state
p ready_slots       # print the whole ready_slots array
p tracked_slot      # print the pointer value
p *tracked_slot     # print the integer pointed to by tracked_slot
```

The call stack explains why this function was running. The local and global values explain why the branch or helper call was reached.

### 4. Reduce Noise with Conditions

Sometimes a value changes many times, and only one change is suspicious. A conditional watchpoint keeps the same watched value but only stops when an extra expression is true.

Useful condition commands:

```gdb
info breakpoints                 # find the watchpoint number
condition 2 state.ticks >= 2     # stop only when this condition is true
condition 2                      # remove the condition from watchpoint 2
```

Watchpoints and breakpoints share the same numbering system. If GDB assigns your watchpoint a number other than `2`, use the number shown by `info breakpoints`.

## Guided Mode

Step 1: Build with debug symbols.

```bash
make
```

Same setup as Lab 01 and Lab 02: make sure `build/lab03` exists and was compiled with debug symbols.

Step 2: Observe both failures outside GDB.

```bash
make run
```

What to look for: Practice 1 changes `ready_count`; Practice 2 changes `ready_slots[1]`.

Step 3: Start GDB.

```bash
gdb ./build/lab03
```

You should see a `(gdb)` prompt.

## Practice 1 Guided Mode: `state.ready_count`

Step 4: Stop before the scheduler mutates global state.

Command:

```gdb
break main
run
```

Meaning: set a breakpoint at `main`, then start execution from a known point.

When to use: when you need to set a watchpoint before the suspicious value changes.

What to look for: GDB stops inside `main` before `run_ready_count_practice` calls `scheduler_tick`.

Step 5: Set a watchpoint on the corrupted field.

Command:

```gdb
watch state.ready_count
```

Meaning: tell GDB to pause whenever `state.ready_count` is written to a different value.

When to use: when you know what value becomes wrong but not who changes it.

What to look for: GDB confirms a hardware watchpoint and gives it a number.

Memory hook: `watch` means "wake me up when this value moves."

Step 6: Continue until the first bad write.

Command:

```gdb
continue
```

Meaning: keep running until the next breakpoint, watchpoint, or program exit.

What to look for: GDB stops when `ready_count` changes from `3` to `2`.

Why this solves the search problem: you did not have to guess whether `rotate_current_process`, `cleanup_finished_processes`, or `printf` was suspicious. The data change itself stopped the program at the writer.

Step 7: Inspect the writer.

Command:

```gdb
frame
list
info locals
p state
p state.ticks
```

Meaning: inspect the current frame, nearby source, local variables, and global scheduler state at the moment of the write.

What to look for: the stop occurs in `cleanup_finished_processes`, on the line that decrements `state.ready_count`.

Step 8: Explain how execution reached the writer.

Command:

```gdb
bt
```

Meaning: show the active call chain that led to the write.

What to look for: the call stack should show `main` calling `run_ready_count_practice`, then `scheduler_tick`, then `cleanup_finished_processes`.

Step 9: Compare the watched field with nearby fields.

Command:

```gdb
p state.ready_count
p state.current_pid
p state.ticks
```

Meaning: inspect the global state around the corrupted field.

What to look for: `state.ticks` is at least `2`, which explains why the guard inside `cleanup_finished_processes` allowed the decrement.

Step 10: Add a condition to stop only when the suspicious guard is true.

Command:

```gdb
info breakpoints
condition 2 state.ticks >= 2
continue
```

Meaning: find the watchpoint number, attach a condition to it, and continue.

When to use: when a watched value changes often and you only care about changes that happen under a specific state.

What to look for: if your watchpoint number is not `2`, replace `2` with the number shown by `info breakpoints`.

Step 11: State the Practice 1 bug.

Command:

```gdb
list cleanup_finished_processes
```

What to look for: `cleanup_finished_processes` decrements `state.ready_count` when `state.ticks >= 2`.

Why it is wrong: in this toy scheduler, no process has actually finished. A tick count alone is not evidence that a process left the ready set, so this cleanup path should not reduce the ready count.

## Practice 2 Guided Mode: `*tracked_slot` and `ready_slots[i]`

Step 12: Restart at Practice 2.

Command pattern:

```gdb
delete
break run_pointer_watch_practice
run
```

Meaning: clear old breakpoints/watchpoints, restart the program, and stop just before the pointer/array scenario runs.

`run` starts the program from the beginning again. The breakpoint on `run_pointer_watch_practice` makes the restarted program stop at the beginning of Practice 2.

If GDB asks:

```text
Delete all breakpoints? (y or n)
```

type:

```gdb
y
```

If GDB asks:

```text
The program being debugged has been started already.
Start it from the beginning? (y or n)
```

type:

```gdb
y
```

What to look for: GDB stops in `run_pointer_watch_practice`. Now set the Practice 2 watchpoint, such as `watch *tracked_slot`.

Step 13: Inspect the pointer and array before setting a watchpoint.

Command:

```gdb
p ready_slots
p tracked_index
p tracked_slot
p *tracked_slot
```

Meaning: confirm that `tracked_slot` points at the slot you care about.

What to look for: `ready_slots` should be `{10, 11, 12}`, `tracked_index` should be `1`, and `*tracked_slot` should be `11`.

Step 14: Watch the value through the pointer.

Command:

```gdb
watch *tracked_slot
continue
```

Meaning: tell GDB to stop when the integer pointed to by `tracked_slot` changes.

What to look for: GDB stops when the value changes from `11` to `-1`.

Why this works: the watchpoint is not watching the pointer variable itself. It is watching the memory reached by dereferencing the pointer.

Step 15: Inspect the pointer writer.

Command:

```gdb
frame
list
info args
bt
```

Meaning: inspect the function that performed the write and how execution reached it.

What to look for: the write happens in `mark_tracked_slot_not_ready`, and the bad operation is `*slot = -1`.

Step 16: Repeat Practice 2 using an array-element expression.

Command pattern:

```gdb
delete
break run_pointer_watch_practice
run
watch ready_slots[tracked_index]
continue
```

Meaning: restart the same scenario, but watch the same integer using array syntax instead of pointer syntax.

What to look for: GDB should stop at the same bad write, because `ready_slots[tracked_index]` and `*tracked_slot` refer to the same memory in this lab.

Step 17: Try the fixed-index version.

Command pattern:

```gdb
delete
break run_pointer_watch_practice
run
watch ready_slots[1]
continue
```

Meaning: watch the exact array element directly.

What to look for: GDB should again stop at `*slot = -1`. The code writes through a pointer, but the array element still changes.

Step 18: State the Practice 2 bug.

Command:

```gdb
list mark_tracked_slot_not_ready
```

What to look for: `mark_tracked_slot_not_ready` should only inspect the tracked slot, but it writes `-1` through the pointer.

Why it is wrong: rebuilding or checking a ready queue should not mark a live slot as not ready. The bug corrupts `ready_slots[1]` indirectly through an alias.

## Hint Mode

Practice 1:

1. Stop at `main`.
2. Watch `state.ready_count`.
3. Continue until GDB stops.
4. Read the old value and new value printed by GDB.
5. Print the call stack.
6. Identify the function and line that performed the write.
7. Inspect `state.ticks` to explain why the branch ran.

Practice 2:

1. Stop at `run_pointer_watch_practice`.
2. Print `ready_slots`, `tracked_slot`, and `*tracked_slot`.
3. Watch `*tracked_slot`.
4. Continue until GDB stops.
5. Print `bt` and `info args`.
6. Repeat with `watch ready_slots[tracked_index]`.
7. Explain why the pointer expression and array expression catch the same write.

## Review Questions

1. What problem does a watchpoint solve better than a normal breakpoint?

   Answer: a watchpoint is useful when you know which value becomes wrong but do not know which code changes it. It stops on the data change instead of requiring you to guess a function or line.

2. How do you stop at the beginning of `main` before setting the Practice 1 watchpoint?

   Answer:

   ```gdb
   break main
   run
   ```

3. How do you watch `state.ready_count` for writes that change its value?

   Answer:

   ```gdb
   watch state.ready_count
   ```

4. What is the difference between `watch`, `rwatch`, and `awatch`?

   Answer: `watch` stops when the expression's value changes, which usually means some code wrote to that memory. `rwatch` stops when the expression is read. `awatch` stops on either read or write access, so it is useful but noisier.

5. After the Practice 1 watchpoint fires, what old/new values should you look for?

   Answer: the first bad change is from `3` to `2`.

6. Which function performs the Practice 1 bad write?

   Answer: `cleanup_finished_processes`.

7. What is the actual Practice 1 bug?

   Answer: `cleanup_finished_processes` uses `state.ticks >= 2` as if it meant a process finished, then decrements `state.ready_count`. In this scenario, no process finished, so `ready_count` should remain `3`.

8. How do you stop at the beginning of Practice 2?

   Answer:

   ```gdb
   break run_pointer_watch_practice
   run
   ```

9. How do you inspect the array and pointer before setting the Practice 2 watchpoint?

   Answer:

   ```gdb
   p ready_slots
   p tracked_slot
   p *tracked_slot
   ```

10. How do you watch the integer pointed to by `tracked_slot`?

    Answer:

    ```gdb
    watch *tracked_slot
    ```

11. How do you watch the same slot using an indexed array expression?

    Answer:

    ```gdb
    watch ready_slots[tracked_index]
    ```

12. How do you watch the same slot using a fixed index?

    Answer:

    ```gdb
    watch ready_slots[1]
    ```

13. Why can `watch *tracked_slot` and `watch ready_slots[1]` stop at the same write?

    Answer: in this lab, `tracked_slot` points to `ready_slots[1]`, so both expressions refer to the same integer in memory.

14. Which function performs the Practice 2 bad write?

    Answer: `mark_tracked_slot_not_ready`.

15. What is the actual Practice 2 bug?

    Answer: `mark_tracked_slot_not_ready` writes `-1` through `*slot`, corrupting `ready_slots[1]` even though that slot should still contain process `11`.
