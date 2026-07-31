# Lab 18：Race、Lock 与 Interrupt Critical Section

预计时间：90–115 分钟。实验采用“未同步/已同步”对照观察，不设置一组
人为 lock bug 让学生排查。

## Part A：观察 Lost Update

两个线程都执行：

```c
value = shared_counter;
thread_yield();
shared_counter = value + 1;
```

两次 increment 的期望值是 2，确定性交错结果是 1：

```text
A load 0
B load 0
A store 1
B store 1
```

在 `counter_loaded` 和 `counter_before_store` 停下，记录：

| 步骤 | running | shared value | local value | ready queue |
|---|---|---:|---:|---|
| A load | | | | |
| B load | | | | |
| A store | | | | |
| B store | | | | |

必须指出哪个 store 覆盖了哪次更新。

## Part B：观察阻塞式 Lock

正确 reference 在完整 read-modify-write 外持锁，并故意在临界区内 yield：

```text
A acquires
→ A yields while owner
→ B tries acquire
→ B becomes WAITING
→ A resumes and releases
→ B becomes READY
→ B rechecks and acquires
```

| Checkpoint | running | owner | B state | ready queue | counter |
|---|---|---|---|---|---:|
| A acquired | | | | | |
| B waits | | | | | |
| A releases | | | | | |
| B acquired | | | | | |

Exercise 补全：

1. `lock_acquire()` 的 `while`、condition wait 和 owner；
2. `lock_release()` 的 owner clear 与 broadcast；
3. 用一个临界区覆盖完整 read-modify-write。

为什么不用 busy-spin：单核 cooperative 系统中，等待者若一直占用 CPU，owner
无法恢复并释放锁。

## Part C：Thread–ISR 共享状态

测试不变量：

```text
event_checksum == event_count * 3
```

checksum 只是暴露中间状态的测试工具，不是同步机制，也不是 ECE391 数据
结构。

未保护路径在两个 update 中间调用 ISR 代理，使 ISR 稳定观察到不一致状态。
保护路径：

```text
old = irq_save_disable()
→ update both fields
→ irq_restore(old)
→ pending ISR may run
```

本实验的 `simulated_isr()` 是确定性教学代理，不是 QEMU 硬件 interrupt
delivery。它验证共享不变量和 `sstatus.SIE` 的 save/restore 语义；Week 5
Lab 15 已负责真实 UART→PLIC→CPU→trap 路径。

Exercise 补全：

1. CSR read-and-clear `sstatus.SIE`；
2. 根据旧值恢复 SIE，而非无条件 enable；
3. 把成组共享状态放进同一 interrupt critical section。

## 运行与 GDB

```bash
make MODE=solution run-check
```

关键断点：

```gdb
break counter_loaded
break counter_before_store
break lock_acquire_observe
break lock_release_observe
break irq_save_disable_observe
break irq_restore_observe
```

预期输出：

```text
[Part A] deterministic race observed
[Part B] lock preserves counter
[Part C] unprotected ISR observation reproduced
[Part C] interrupt state restored
LAB18 PASS
```

## 复习题

1. cooperative threading 为什么仍会 race？
2. 为什么只锁 load 或只锁 store 不够？
3. broadcast 后为什么必须用 `while` 重查 owner？
4. 为什么阻塞 lock 不能从 ISR 调用？
5. 为什么退出 interrupt critical section 时不能无条件 enable？
6. 多核系统中只关闭当前 hart interrupt 为什么不够？
