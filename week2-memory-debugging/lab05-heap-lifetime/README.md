# Lab 05: Heap Lifetime and Use-After-Free

## Goal

Use `print ptr`, `ptype ptr`, `x/32xb ptr`, and watchpoints to reason about heap object lifetime. Use AddressSanitizer as a runtime checker that confirms whether a suspicious heap access is a real memory-lifetime bug.

## Failure Scenario

Run the normal build:

```bash
make run
```

You may see output similar to:

```text
Before free: id=391 owner=shell state=1 at 0x55...
Released active request, but cached_request still equals 0x55...
After stale write: state=0 at 0x55...
Expected: do not read or write a request after free.
```

The exact values after `free` may vary. That uncertainty is part of the bug: once an object is freed, reading or writing it is undefined behavior.

Now run the AddressSanitizer build:

```bash
make asan
```

AddressSanitizer should report a `heap-use-after-free` near `mark_cached_request_done`.

The bug is that `cached_request` still points to the heap allocation after `release_request(active)` frees it. The pointer value is not `NULL`, but the object lifetime is over.

## Where This Shows Up / Common Scenarios

This pattern appears when code keeps a pointer in a global table, process control block, file descriptor object, request cache, or cleanup list after freeing the original object.

In ECE391-style systems code, a stale pointer can look like a valid address while pointing to memory that now belongs to something else.

## Concept Warmup

This lab uses a pointer-lifetime debugging loop:

```text
classify with ASan -> inspect pointer -> inspect type and bytes -> find where lifetime ended -> explain the stale access
```

### Heap Object Lifetime / 堆对象生命周期

`malloc` creates a heap object. `free` ends that object's lifetime.

After `free(req)`, the old pointer may still contain the same address:

```text
req == 0x5555555592a0
cached_request == 0x5555555592a0
```

That does not mean the object is still valid. It only means the pointer variable still stores an address.

Why might the normal run not crash?

`free(req)` tells the allocator that the heap object is no longer owned by your code. It does not necessarily make the old address unreadable immediately. The operating system often still has that memory page mapped, so the CPU may allow the access even though the C object lifetime is already over.

Possible outcomes after use-after-free:

- If the freed memory has not been reused yet, the program may appear to work.
- If another `malloc` reuses the same memory, the stale pointer may corrupt a different object.
- If the allocator overwrites freed-chunk metadata, the stale pointer may read strange values.
- If the access reaches an unmapped page, the program may crash with `Segmentation fault`.
- If ASan, Valgrind, or a similar runtime checker is enabled, the tool may report the invalid access.

Important point / 重点: no crash does not mean no bug. Use-after-free is already wrong once the program reads or writes an object after its lifetime ended.

### Pointer Type / 指针类型

`ptype` shows what kind of object a pointer claims to point to:

```gdb
ptype cached_request
ptype *cached_request
```

This helps you remember the expected layout before inspecting raw bytes.

### Memory Bytes / 内存字节

Use `x/32xb ptr` when you want the raw byte view:

```gdb
x/32xb cached_request
```

Use a typed print when you want the C struct view:

```gdb
p *cached_request
```

Both views can be useful, but neither makes a freed object safe to use.

### AddressSanitizer / 地址消毒器

AddressSanitizer, often shortened to ASan, is a runtime memory-error detector. It is not static analysis.

Static analysis reads code without running the program. ASan works differently:

```text
compile with -fsanitize=address -> run the program -> ASan checks real memory accesses
```

In this lab, `make asan` compiles the program with:

```bash
-fsanitize=address -fno-omit-frame-pointer
```

This is defined in this lab's `Makefile`; `make asan` is not a built-in Make command. The relevant lines are:

```makefile
ASAN_CFLAGS := $(CFLAGS) -fsanitize=address -fno-omit-frame-pointer
ASAN_TARGET := $(BUILD_DIR)/lab05-asan

asan: $(ASAN_TARGET)
	ASAN_OPTIONS=detect_leaks=0 ./$(ASAN_TARGET)
```

So `make asan` means: build a separate ASan-instrumented executable, then run that executable with ASan runtime options.

The extra instrumentation lets ASan track heap object lifetime:

```text
malloc: object is alive
free:   object lifetime is over
write:  if code writes through the old address, report heap-use-after-free
```

Use `make asan` when you suspect:

- use-after-free / 释放后继续使用
- double free / 重复释放
- heap buffer overflow / 堆数组或缓冲区越界
- stack or global buffer overflow / 栈或全局缓冲区越界
- a crash that depends on allocator layout, timing, or memory reuse
- a pointer that still has an address but may no longer point to a live object

ASan is especially useful before a long GDB session because it can name the memory-error category and show three useful stack traces:

```text
where the bad access happened
where the object was freed
where the object was allocated
```

Limitations / 局限:

- ASan only checks code paths that actually run.
- ASan does not prove the whole program is memory-safe.
- ASan does not replace GDB. Use ASan to identify the memory error, then use GDB to inspect the program state and understand why the bad pointer survived.

## Guided Mode

Step 1: Build and observe the normal run.

```bash
make
make run
```

What to look for / 看什么: the program continues after `free`, but that does not mean it is correct.

Step 2: Build and run the ASan version.

```bash
make asan
```

Meaning / 是什么: compile with AddressSanitizer and run the program with runtime memory checks. This is not static analysis. The program really runs, and ASan stops it when the instrumented code performs an invalid memory access.

When to use / 什么时候用: use ASan when the symptom smells like a memory bug: use-after-free, double free, heap overflow, stack overflow, or a crash that changes when allocation order changes. In this lab, use it because `cached_request` still has an address after `free`, but the object may no longer be alive.

What to look for / 看什么: first read the error type and access type:

```text
ERROR: AddressSanitizer: heap-use-after-free
WRITE of size 4
```

Then read the three stack traces:

```text
bad access:          mark_cached_request_done
freed by:            release_request
previously allocated: create_request
```

Why this helps / 为什么有用: ASan gives you the heap lifetime timeline before you start single-stepping in GDB.

Common scenario / 常见场景: run ASan when a pointer value is non-NULL but you are not sure whether the pointed object is still valid.

Memory hook / 记忆钩子: ASan answers "did this memory access touch an object that is still alive?"

Step 3: Start GDB and stop before `free`.

```bash
gdb ./build/lab05
```

```gdb
break release_request
run
```

Meaning / 是什么: pause at the function that ends the heap object's lifetime.

Step 4: Inspect the pointer and its type.

```gdb
p cached_request
p *cached_request
ptype cached_request
ptype *cached_request
```

What to look for / 看什么: `cached_request` points to a `struct request`, and the struct still contains `id`, `owner`, and `state` before the free.

Step 5: Inspect raw heap memory before the free.

```gdb
x/32xb cached_request
```

Meaning / 是什么: show 32 bytes starting at the heap address.

Why this helps / 为什么有用: raw memory lets you compare the same address before and after `free`.

Step 6: Step over the free and inspect again.

```gdb
next
p cached_request
x/32xb cached_request
```

What to look for / 看什么: the pointer value is still non-NULL. Some bytes may already be changed by the allocator.

Important point: a non-NULL pointer is not proof that the pointed object is alive.

Step 7: Stop at the stale write.

```gdb
break mark_cached_request_done
continue
bt
list
```

What to look for / 看什么: the program is about to write `cached_request->state = 0` after the object was freed.

Step 8: Inspect the stale pointer at the write site.

```gdb
p cached_request
p cached_request->state
x/32xb cached_request
```

Meaning / 是什么: inspect the same stale heap address immediately before the use-after-free write.

Step 9: Use a watchpoint as a comparison.

Restart the program:

```gdb
delete
break release_request
run
watch cached_request->state
continue
```

What to look for / 看什么: GDB should stop when `mark_cached_request_done` changes the `state` field.

Common scenario / 常见场景: use a watchpoint when a freed object or table entry changes and you need to know who wrote it.

Step 10: State the bug.

The bug is not that the pointer became `NULL`. It did not. The bug is that the program kept using `cached_request` after `free(active)` ended the heap object's lifetime.

## Hint Mode

1. Run `make asan` and read the sanitizer error type.
2. In GDB, stop at `release_request`.
3. Print `cached_request` and `*cached_request`.
4. Use `ptype` to confirm the struct layout.
5. Examine 32 bytes at the pointer address.
6. Step over `free`.
7. Stop at `mark_cached_request_done`.
8. Explain why the pointer value is not enough to prove the object is valid.

## Review Questions

1. What command prints the pointer value stored in `cached_request`?

   Answer:

   ```gdb
   p cached_request
   ```

2. What command prints the struct that `cached_request` points to?

   Answer:

   ```gdb
   p *cached_request
   ```

3. What command shows the type of the pointed-to object?

   Answer:

   ```gdb
   ptype *cached_request
   ```

4. How do you inspect 32 raw bytes starting at the pointer address?

   Answer:

   ```gdb
   x/32xb cached_request
   ```

5. Why is `cached_request != NULL` not enough?

   Answer: the pointer variable can still hold an address after `free`, but the heap object at that address is no longer valid to read or write.

6. What tool reports `heap-use-after-free` for this lab?

   Answer: AddressSanitizer, run with `make asan`.

7. Is ASan static analysis?

   Answer: no. ASan instruments the program at compile time and checks real memory accesses while the program runs.

8. When should you reach for `make asan`?

   Answer: use it when you suspect a runtime memory bug such as use-after-free, double free, heap overflow, stack overflow, or a pointer that may outlive the object it points to.

9. What three places should you look for in an ASan use-after-free report?

   Answer: the bad access site, the free site, and the original allocation site.

10. Which function ends the heap object's lifetime?

   Answer: `release_request`, because it calls `free(req)`.

11. Which function performs the stale write?

   Answer: `mark_cached_request_done`.

12. What is the actual bug?

   Answer: the program keeps using `cached_request` after freeing the heap object it points to.
