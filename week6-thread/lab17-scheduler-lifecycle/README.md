# Lab 17：首次启动、Round-Robin 与 Condition Wait

预计时间：90–115 分钟。先运行 `MODE=solution`，完成观察表后再修改
`MODE=exercise`。

## 学习目标

- 构造新线程的 artificial context；
- 解释 `_swtch → thread_setup → worker → thread_exit`；
- 观察 FIFO ready queue 和 RUNNING/READY/WAITING/EXITED；
- 区分 `thread_yield()` 与 `condition_wait()`；
- 解释 main、worker 和 idle thread；
- 比较 cooperative 与 preemptive scheduling。

## 首次启动

新线程从未调用过 `_swtch()`，因此没有旧 `ra/sp` 可恢复。`thread_spawn`
必须构造：

```text
context.sp = aligned stack high
context.ra = thread_setup
start_fn   = worker
start_arg  = arg
state      = READY
```

第一次被选中后：

```text
_swtch restores artificial context
→ ret to thread_setup
→ call worker(arg)
→ worker returns
→ thread_exit
→ EXITED and switch away permanently
```

## Round-Robin

```text
running A, ready [B, C, main]
→ A: RUNNING→READY
→ append A at tail
→ remove B from head
→ B: READY→RUNNING
```

Cooperative threading 依赖线程主动 yield。Preemptive scheduling 可由 timer
interrupt 强制切换，因此会扩大需要同步保护的位置；本实验不实现 timer
preemption。

## Condition Wait

`yield` 后线程仍是 READY；`condition_wait` 后线程是 WAITING，不能留在
ready queue。

```text
waiter checks event_ready == false
→ condition_wait
→ RUNNING→WAITING
→ leave runnable set
→ device/ISR-facing helper broadcasts
→ WAITING→READY
→ append to ready queue
→ scheduler later resumes waiter
```

ECE391 接口允许 ISR 调用 `condition_broadcast()`，但 ISR 不可调用
`condition_wait()`；broadcast 只改变 waiter 状态，不立即 yield。

本实现先使用确定性设备事件，确保自动测试不依赖人工输入。事件发布只通过
`condition_broadcast()`，不直接修改 waiter 或 ready queue；它对应真实 UART
ISR 收到字节后的同一状态边界。

## 运行 Reference

```bash
make MODE=solution
make MODE=solution run-check
```

预期：

```text
[waiter] resumed after broadcast
LAB17 PASS
```

## GDB Checkpoints

```gdb
break thread_created
break thread_setup_entry
break scheduler_before_switch
break scheduler_after_select
break thread_function_returned
break thread_marked_exited
break condition_wait_entered
break condition_thread_sleeping
break condition_broadcast_done
break condition_thread_resumed
```

### 首次启动

| Checkpoint | TID | `tp` | `sp`/stack range | `ra` | state |
|---|---:|---:|---|---|---|
| created | | | | | |
| setup entry | | | | | |
| worker entry | | | | | |

### RR 队列

| running | yield 前 ready | 入队后 | selected next | yield 后 ready |
|---|---|---|---|---|
| main | | | | |
| A | | | | |
| B | | | | |

### Condition

| Checkpoint | running | waiter state | ready queue | wait list |
|---|---|---|---|---|
| before wait | | | | |
| sleeping | | | | |
| after broadcast | | | | |
| resumed | | | | |

## Exercise

未完成代码停在 `lab17_todo_checkpoint()`。完成三项：

1. `thread_spawn()`：设置对齐的 `sp`、`thread_setup`、函数/参数和 READY；
2. `thread_yield()`：当前线程入队尾、next 从队首取出；
3. `thread_exit()`：设置 EXITED 并永久切换。

框架提供正确 `_swtch()`、thread table、stack storage、ready-list primitive 和
只读 condition 实现。完整 condition 实现是 Lab 18 后的 Challenge。

## 验收与复习

```bash
make MODE=solution check
make MODE=solution run-check
```

回答：

1. 为什么新线程的 `ra` 应指向 `thread_setup` 而不是零？
2. worker 返回后为什么必须进入 `thread_exit()`？
3. WAITING 与 READY 在线程可调度性上有什么区别？
4. broadcast 后 waiter 为什么不保证立即运行？
5. ready list 为空时为什么需要永不退出的 idle thread？
