# Lab 16：观察并实现 Cooperative Context Switch

预计时间：80–100 分钟。

## 核心问题

单个 CPU 怎样从线程 A 切换到线程 B，并让两个线程都认为自己的
`_swtch()` 调用正常返回？

本实验使用真实 RV64 汇编和两个独立内核栈，不使用 `pthread`。先运行
正确 reference 收集数据，再完成 exercise。不要先猜 offset。

## ECE391 要掌握什么

完成后应能解释和编写：

- `struct thread_context` 中的 `ra`、`sp`、`s0-s11`；
- 用旧 `tp` 保存 current context；
- 保存完成后令 `tp = next`；
- 用新 `tp` 恢复 next context；
- `ret` 为什么进入另一个线程的旧执行点。

本实验不包括 thread spawn、scheduler、preemption 或 condition variable。

## Thread Context 与 Trap Frame

| Thread context | Trap frame |
|---|---|
| cooperative switch 是显式函数调用 | trap 可在任意指令间发生 |
| ABI 已规定 caller-saved register 可被调用覆盖 | 被中断代码没有主动调用 handler |
| 保存 `ra/sp/s0-s11` 即可恢复 C 调用环境 | 通常必须保存完整通用寄存器状态 |

`tp` 表示当前 thread object，`sp` 表示当前调用链。二者不是同一个概念。

## 正确执行路径

```text
A calls _swtch(B)
  → old tp points to A
  → save A.ra/sp/s0-s11
  → tp = B
  → restore B.ra/sp/s0-s11
  → ret uses B.ra
  → B continues

B calls _swtch(A)
  → save B
  → restore A
  → A's original _swtch returns
```

第一次进入 B 所需的初始 context 已由框架构造。如何构造新线程是 Lab 17
的主题。

## Part A：运行 Reference

```bash
make MODE=solution
make MODE=solution run
```

预期：

```text
[A] before switch
[A] resumed with its own stack and s-registers
LAB16 PASS
```

输出没有逐条打印 B 的执行过程，因为 UART 调用会干扰寄存器观察。B 的真实
执行通过 GDB checkpoint 验证。

## Part B：GDB 观察

终端一：

```bash
make MODE=solution debug
```

终端二：

```bash
make MODE=solution gdb
```

设置断点：

```gdb
break thread_a_before_switch
break swtch_save_done
break swtch_tp_changed
break swtch_restore_done
break thread_b_after_switch
break thread_a_after_resume
continue
```

每次停止后执行：

```gdb
info registers tp sp ra s0 s1
p/x thread_a
p/x thread_b
x/14gx &thread_a.context
x/14gx &thread_b.context
bt
```

填写：

| Checkpoint | `tp` | `sp` 属于哪个栈 | `ra` | context memory 说明 |
|---|---|---|---|---|
| A before | | | | |
| save done | | | | |
| `tp` changed | | | | |
| restore done | | | | |
| B after `ret` | | | | |
| A resumed | | | | |

重点回答：

1. `swtch_tp_changed` 时为什么 `tp` 已属于 B，而 `sp` 仍可能属于 A？
2. `swtch_restore_done` 后的 `ra` 来自哪个 object？
3. 为什么 `ret` 后不是回到 A？

## Part C：完成 Exercise

```bash
make MODE=exercise
make MODE=exercise run
```

未完成版本会明确停在 `lab16_todo_checkpoint()`，不会用损坏的 context
继续执行。

在 `thread.h` 和 `swtch.S` 完成四项：

1. 确认 context layout；
2. 用旧 `tp` 保存 `ra/sp/s0-s11`；
3. 保存完成后更新 `tp`；
4. 用新 `tp` 恢复并 `ret`。

保存与恢复必须使用相同 slot。不要在保存 current 之前覆盖 `tp`。

## 验收

```bash
make MODE=solution check
make MODE=solution run-check
```

最终实现还应在 GDB 中证明：

- A/B 的 stack range 不同；
- `s1` 的 `0xa16` 与 `0xb16` sentinel 各自保持；
- A→B→A 的 `tp` 与 `sp` 属于同一 running thread；
- save/restore layout 对称。

## 推理题

1. 如果先执行 `tp = next` 再保存，A 的寄存器会写到哪里？
2. 如果 `s1` save 和 restore 使用不同 offset，错误何时才可见？
3. 为什么本实验不保存 `a0-a7` 和 `t0-t6`？
4. 如果改成 timer-driven preemption，当前最小 context 是否足够？
