# Lab 04: Array Overflow and Nearby Memory

## Goal

Use `print &var`, `x/16xw addr`, `x/16dw addr`, `bt`, and `info registers` to prove that an out-of-bounds array write corrupted nearby stack data.

## Failure Scenario

Run the program:

```bash
make run
```

You should see that `guard_word` changes after importing scores:

```text
Before import:
scores: [0, 0, 0, 0]
guard_word: 0x39139139
student_id: 391

After import:
scores: [80, 90, 85, 95]
guard_word: 0x64
student_id: 391
Expected guard_word to stay 0x39139139.
```

The program does not crash. That is the point of the lab: memory corruption often becomes visible as a wrong nearby value before it becomes a segmentation fault.

The bug is in `import_scores`. It imports five integers into `packet->scores`, but that array has only four elements. The fifth write, `scores[4] = 100`, lands on `packet->guard_word`.

## Where This Shows Up / Common Scenarios

This pattern appears when a table, buffer, command array, file descriptor list, process list, or paging-related array accepts one element too many.

In ECE391-style code, the symptom might be a field beside the array changing unexpectedly, a later crash in unrelated code, or a data structure that looks correct except for one strange value.

## Concept Warmup

This lab uses a memory-layout debugging loop:

```text
find nearby variables -> print their addresses -> inspect raw memory -> stop at the bad write
```

### Array Bounds / 数组边界

`scores[0]` through `scores[3]` are valid for an array of four integers.

`scores[4]` is one element past the end. C will not automatically stop the write. If another variable or struct field lives at that address, the write corrupts that nearby value.

### Address / 地址

An address is the memory location where a value is stored.

Useful address commands:

```gdb
p packet
p &packet->scores
p &packet->scores[0]
p &packet->scores[4]
p &packet->guard_word
```

If `&packet->scores[4]` and `&packet->guard_word` are the same address, then `scores[4]` is another name for the guard field's memory.

### Examine Memory / 查看内存

`x` means examine memory.

Common forms:

```gdb
x/8dw packet->scores      # 8 decimal words
x/8xw packet->scores      # 8 hexadecimal words
x/16xb packet->scores     # 16 bytes in hex
```

For this lab, `w` means a 4-byte word, which matches `int` on the target environment.

### CPU and Stack Context / CPU 和栈上下文

`info registers` prints the CPU register snapshot at the current stop point. It is noisy on purpose: GDB shows general-purpose registers, the instruction pointer, flags, segment registers, and sometimes extra CPU-specific registers.

For this lab, do not try to understand every line. Read it in this order:

```text
1. frame / info args tells you the source-level truth first.
2. rip tells you where the CPU is executing.
3. rsp and rbp tell you where the current stack frame is.
4. argument-like registers may echo the function arguments, but they are secondary.
```

`rsp / stack pointer / 栈指针`: points to the current top of the stack. Use it when you want to inspect raw stack memory:

```gdb
p/x $rsp
x/16xw $rsp
```

`rbp / base pointer / 栈帧基准指针`: usually points near the base of the current function's stack frame when compiling with `-O0`. Think of it as an anchor for the current function call.

Simple stack picture / 简化栈图:

```text
High address
+-------------------------+
| caller's stack frame    |
| return address          |
+-------------------------+
| saved old rbp           | <- rbp often points near here
| local variables         |
| temporary stack data    | <- rsp points around current stack top
+-------------------------+
Low address
```

Quick memory hook / 记忆钩子:

```text
rsp: stack top, 现在栈顶在哪里
rbp: frame base, 当前函数这一层栈帧的基准点
```

When this lab stops inside `write_score`, you should expect `$rsp` and `$rbp` to look like stack addresses, usually starting with something like `0x7ffff...` on x86_64 Linux. They should be close to each other because this function is small. In your run they may even be equal if GDB stops near the end of the function.

Do not expect `$rsp` or `$rbp` to equal `scores`. `scores` points into the caller's `packet` object, while `$rsp` and `$rbp` describe the current `write_score` stack frame.

Why can `$rsp == $rbp` here?

`write_score` is tiny:

```c
static void write_score(int *scores, int index, int value) {
    scores[index] = value;
}
```

It has no local array and does not need much temporary stack space. When compiled with `-O0`, the function may set up a simple frame and then restore the stack by the time GDB stops near the closing brace. Seeing `$rsp` and `$rbp` equal or very close is normal for this lab.

Why are the arguments not sitting on the stack?

On x86_64 Linux, the first six integer or pointer arguments usually go through registers:

```text
1st arg -> rdi
2nd arg -> rsi
3rd arg -> rdx
4th arg -> rcx
5th arg -> r8
6th arg -> r9
```

For this lab:

```text
write_score(scores, index, value)

scores -> rdi
index  -> rsi
value  -> rdx
```

By the time you stop at the end of the function, some registers may already have been reused, so `info args` is still the safer beginner view. The register mapping is useful context, not the first thing to trust.

Example:

```text
#0 write_score (scores=0x7fffffffe130, index=4, value=100) at main.c:11

rdi  0x7fffffffe130
rsi  0x4
rax  0x64
rbp  0x7fffffffe0d0
rsp  0x7fffffffe0d0
rip  0x5555555551b4 <write_score+43>
```

What matters here:

- `frame` already says `scores=0x7fffffffe130`, `index=4`, and `value=100`.
- `rip` says execution is still inside `write_score`.
- `rsp` is the current stack pointer. `x/16xw $rsp` starts dumping raw stack memory from that address.
- `0x64` is decimal `100`, which matches the value being written.

`x/16xw $rsp` means:

```text
x   examine memory
/16 show 16 units
x   print each unit in hexadecimal
w   each unit is one 4-byte word
$rsp start at the current stack pointer
```

Use this command to get comfortable seeing stack memory as raw words. In this lab, it is supporting evidence, not the main proof. The main proof is still the address comparison: `&packet->scores[4]` overlaps `&packet->guard_word`.

Visual mental model:

```text
packet on the stack

scores[0]   scores[1]   scores[2]   scores[3]   guard_word   student_id
   80          90          85          95           100          391
                                                   ^
                                                   scores[4] wrote here
```

## Guided Mode

Step 1: Build and observe the failure.

```bash
make
make run
```

What to look for / 看什么: `guard_word` changes from `0x39139139` to `0x64`. Decimal `100` is hexadecimal `0x64`, which matches the fifth input score.

Step 2: Start GDB and stop before the import.

```bash
gdb ./build/lab04
```

```gdb
break import_scores
run
```

Meaning / 是什么: pause before the array writes happen.

When to use / 什么时候用: when a nearby value is wrong after a loop or copy, stop before the mutation and inspect the layout.

Step 3: Print addresses inside the packet.

```gdb
p packet
p &packet->scores[0]
p &packet->scores[1]
p &packet->scores[2]
p &packet->scores[3]
p &packet->scores[4]
p &packet->guard_word
```

What to look for / 看什么: `&packet->scores[4]` should match `&packet->guard_word`.

Why this helps / 为什么有用: it proves that the fifth array slot is not real array storage. It is the next field in the struct.

Step 4: Inspect the same area as raw memory.

```gdb
x/8dw packet->scores
x/8xw packet->scores
```

Meaning / 是什么: show the memory starting at the first score as decimal words and hexadecimal words.

What to look for / 看什么: before the import, the first four words are `0`, then the guard word appears as `0x39139139`.

Step 5: Stop exactly on the out-of-bounds write.

```gdb
break write_score
info breakpoints
condition 2 index == 4
continue
```

If your breakpoint is not number `2`, replace `2` with the number shown by `info breakpoints`.

Meaning / 是什么: stop only when `write_score` is called with the suspicious index.

What to look for / 看什么: GDB should stop with `index = 4` and `value = 100`.

Step 6: Inspect the bad call.

```gdb
bt
info args
info locals
p scores
p scores[index]
x/8dw scores
```

What to look for / 看什么: `scores[index]` is the memory that overlaps the guard field.

Step 7: Inspect CPU and stack context.

```gdb
frame
info args
p/x $rsp
p/x $rbp
info registers
x/16xw $rsp
```

Meaning / 是什么: inspect the machine-level state around the current function. `p/x $rsp` and `p/x $rbp` print the two stack-related registers in hexadecimal. `info registers` shows the full CPU register set. `x/16xw $rsp` shows 16 hexadecimal 4-byte words starting at the current stack pointer.

When to use / 什么时候用: use this when source-level variables are not enough, or when stack corruption may affect return addresses and saved registers.

What to look for / 看什么: start with the short source-level lines, then use registers as context.

Example stop point:

```text
#0  write_score (scores=0x7fffffffe130, index=4, value=100) at main.c:11
```

This is the clearest line. It says the current function received:

```text
scores = 0x7fffffffe130
index  = 4
value  = 100
```

In the register output, focus on these lines:

```text
rip  0x5555555551b4 <write_score+43>
rsp  0x7fffffffe0d0
rbp  0x7fffffffe0d0
rdi  0x7fffffffe130
rsi  0x4
rax  0x64
```

How to read them:

- `rip` is the instruction pointer. It says the CPU is currently executing inside `write_score`.
- `rsp` is the stack pointer. It is the address used by `x/16xw $rsp`. In this lab, expect it to look like a stack address near `0x7ffff...`.
- `rbp` is often the current stack frame base when compiling with `-O0`. In this lab, expect it to be close to `$rsp`; it may be equal when stopped near the end of the tiny `write_score` function.
- `rdi` and `rsi` often hold early function arguments on x86_64 Linux. Here they line up with `scores` and `index`.
- `0x64` is hexadecimal for `100`, which matches the value being written.

Why `$rsp == $rbp` is okay here / 为什么这里相等也正常:

`write_score` has no local buffer and no meaningful local variables. It does not need a large stack frame. If GDB stops near the closing brace, the function's stack usage may already be minimal, so `$rsp` and `$rbp` can be equal.

Why arguments are not on the stack / 为什么参数不在栈里:

On x86_64 Linux, the first six integer or pointer arguments usually use registers:

```text
rdi, rsi, rdx, rcx, r8, r9
```

That is why `scores` and `index` line up with `rdi` and `rsi` instead of appearing as obvious words at `$rsp`.

Expected shape / 期望看到的形状:

```text
$rsp and $rbp: stack-looking addresses, close to each other
scores:        another stack-looking address, but usually higher than this frame
index:         4
value:         100 / 0x64
```

Red flags / 可疑信号:

- `$rsp` is `0x0`.
- `$rsp` points to a tiny integer-looking value such as `0x64`.
- `$rsp` or `$rbp` points into code memory near `rip`, such as `0x5555...`.
- `x/16xw $rsp` fails because the stack pointer points to unreadable memory.

Those red flags are not expected in this lab, but they are the kind of thing you would worry about in a real stack-corruption bug.

Do not rely on registers before `info args`. Registers are reused constantly. In beginner source-level debugging, first trust:

```gdb
frame
info args
info locals
```

Then use `info registers` to connect the C-level story to the CPU-level story.

Example stack dump shape:

```text
(gdb) x/16xw $rsp
0x7fffffffe0d0: 0xffffe100  0x00007fff  0x555551f0  0x00005555
0x7fffffffe0e0: 0xffffe130  0x00007fff  0x00000004  0x00000064
```

Do not memorize these exact numbers. Stack addresses change between runs. The useful habit is to recognize that GDB is showing raw 4-byte chunks near the current stack pointer.

On 32-bit x86, use:

```gdb
x/16xw $esp
```

Step 8: Finish the function and confirm the corruption.

```gdb
finish
p packet->guard_word
x/8dw packet->scores
```

What to look for / 看什么: the guard word now contains `100`.

## Hint Mode

1. Run the program and identify which field changes unexpectedly.
2. Stop at `import_scores`.
3. Print the addresses of `scores[0]`, `scores[4]`, and `guard_word`.
4. Inspect memory starting at `packet->scores`.
5. Break on `write_score`.
6. Add a condition so GDB stops only when `index == 4`.
7. Use `bt` and `info args` to explain where the bad index came from.

## Review Questions

1. What is the valid index range for `scores[4]` when `scores` has four elements?

   Answer: valid indexes are `0`, `1`, `2`, and `3`.

2. How do you print the address of `packet->guard_word`?

   Answer:

   ```gdb
   p &packet->guard_word
   ```

3. How do you inspect eight decimal words starting at `packet->scores`?

   Answer:

   ```gdb
   x/8dw packet->scores
   ```

4. How do you inspect the same memory as hexadecimal words?

   Answer:

   ```gdb
   x/8xw packet->scores
   ```

5. Why is `scores[4]` dangerous in this lab?

   Answer: `scores[4]` is one element past the end of the four-element array, and it overlaps `guard_word`.

6. How do you stop only when `write_score` receives `index == 4`?

   Answer:

   ```gdb
   break write_score
   info breakpoints
   condition 2 index == 4
   continue
   ```

   Replace `2` with your actual breakpoint number.

7. What command shows the call path that reached the bad write?

   Answer:

   ```gdb
   bt
   ```

8. What is the actual bug?

   Answer: `import_scores` copies five incoming scores into a four-element destination array.
