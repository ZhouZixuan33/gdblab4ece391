# Week 6 内核线程实验教学设计

## 1. 定位与范围

本组实验对应 ECE391-L7、L8、L9，使用现有 QEMU `virt`、RV64 裸机内核和 GDB remote debugging 环境，不使用 Linux `pthread`。三个实验分别回答：

1. Lab 16：CPU 如何从一个线程切换到另一个线程；
2. Lab 17：内核如何创建、调度和结束线程；
3. Lab 18：并发执行流如何安全访问共享状态。

每个实验预计 1–2 小时。Week 5 的 boot、UART、trap 和链接基础设施作为框架复用。学生不从零编写完整线程库，而是完成决定概念正确性的关键代码，并通过 GDB 收集证据。

本组实验不包含：

- Linux `pthread`；
- timer-driven preemption；
- 多 hart 调度；
- 多核 memory ordering；
- 页表和进程地址空间；
- 完整用户进程系统；
- 高性能 spinlock；
- deadlock detection；
- nested interrupt。

Condition wait/broadcast 的状态流观察属于 Lab 17 必做路径；condition variable 的完整代码实现作为 Lab 18 Challenge。

## 2. 教学方法

三个实验都要求学生先建立运行模型，再接触代码。Lab 16 和 Lab 17 使用可运行的正确基线，以流程和真实数据观察为主；Lab 18 使用确定性 interleaving 暴露 race，因为“观察到错误结果并解释交错”本身就是该实验的核心概念。

1. 学生先预测运行结果和状态变化；
2. 运行 reference 或 exercise，获得可重复的基线现象；
3. 在稳定 checkpoint 停下；
4. 使用 GDB 检查寄存器、线程结构、栈和队列；
5. 在已理解流程后补全少量关键代码；
6. 重新运行并解释代码补全或修复的因果链；
7. 使用自动脚本验证多轮执行结果。

使用故障的实验必须保证故障可重复、可定位，不依赖偶然时序或不可解释的随机崩溃。Lab 17 不以故障诊断作为学习目标。

## 3. 重要概念与掌握程度

| 概念 | 必须理解 | 必须实操 | 不要求 |
|---|---|---|---|
| Program/process/thread | 同一进程的线程共享代码、静态区和堆，但各有寄存器与栈 | 区分两个线程的 `sp`、栈和执行位置 | 地址空间隔离 |
| Thread context | 恢复后应像未被暂停 | 检查 `ra`、`sp`、`s0-s11` | FP/vector context |
| Calling convention | cooperative `_swtch()` 重点保存 callee-saved registers | 对照 C layout 补全 `sd/ld` | 背诵全部 ABI |
| Context switch | 保存 A、切换 `tp`、恢复 B、`ret` 到 B | 证明“A 调用、B 返回” | timer preemption |
| 独立线程栈 | 切换 `sp` 即切换调用链 | 验证 stack range | guard page |
| 首次启动 | 新线程需要人工构造初始 context | 设置 `sp`、trampoline 和参数 | 完整通用 varargs |
| Round-Robin | 队首取下一个，yield 线程入队尾 | 跟踪 `A→B→C→A` | priority/aging |
| Thread state | WAITING/EXITED 不可被调度 | 验证状态转换 | 完整资源回收 |
| Race condition | C 语句可能是多条机器指令 | 重现并解释 lost update | 形式化证明 |
| Critical section | 围绕共享不变量定义 | 找出最小正确临界区 | lock-free algorithm |
| Lock | 保证 thread-thread mutual exclusion | 修复 race 并验证 owner | 高性能多核锁 |
| Interrupt masking | 保存旧状态、关闭、恢复，处理 thread-ISR 并发 | 编写 CSR helper | SMP interrupt coordination |
| Condition variable | wait 移出 runnable 集合，broadcast 使其 READY；wait 不可在 ISR 调用，broadcast 可在 ISR 调用 | Lab 17 跟踪 RUNNING→WAITING→READY→RUNNING | 完整实现为 Challenge |
| Deadlock | 能识别持锁等待和锁顺序问题 | 识别简单故障 | 检测与恢复 |

## 4. ECE391 代码能力要求

| 模块 | 学生最终应达到的代码能力 |
|---|---|
| `struct thread_context` | 独立定义 `ra`、`sp`、`s0-s11`，保证 C/汇编 offset 一致 |
| `_swtch()` | 在给定 ABI 和 layout 后，独立完成保存、切换 `tp`、恢复和 `ret` |
| `struct thread` | 使用 context、stack、state、id、parent 和 list link |
| `thread_spawn()` | 分配 entry/stack，构造初始 context，将线程加入 ready queue |
| Thread trampoline | 解释并补全“首次 restore→thread function→thread_exit” |
| `thread_yield()` | 完成 READY/RUNNING 转换和 RR 入队、出队 |
| `thread_exit()` | 标记 EXITED、唤醒等待者并永久切走 |
| RR scheduler | 独立实现最小 FIFO ready queue 调度 |
| Condition wait/broadcast | 在给定接口和 interrupt 规则后补全核心状态转换 |
| Lock | 定义 owner/waiters，补全 acquire/release |
| Interrupt critical section | 独立实现 save-disable-restore |
| 并发测试 | 编写共享状态测试和可检查的不变量 |
| 调试 | 设置 checkpoint/sentinel，用 GDB 验证机器状态 |

“应能编写”不代表每段代码都必须在短实验中从零完成。实验选择关键片段让学生实际编写，其余通过阅读、调试和 Review Questions 考核。

# Lab 16：线程现场与 `_swtch()`

## 5. 核心问题与目标

核心问题：

> 单个 CPU 怎样在两个线程之间切换，并让每个线程认为自己的函数调用从未被打断？

预计时间 75–110 分钟。完成后学生应能够：

- 区分 thread context 与 trap frame；
- 解释 cooperative switch 保存 `ra`、`sp`、`s0-s11` 的原因；
- 将 `struct thread_context` 字段与汇编 offset 对应；
- 观察 `tp`、`sp`、`ra` 的切换；
- 验证两个线程使用独立栈；
- 解释 A 调用的 `_swtch()` 为什么能在 B 中返回；
- 根据观察到的流程实现对称的保存与恢复。

## 6. 观察优先的任务与框架边界

Lab 16 先提供可直接运行的正确 reference。学生在修改代码前完整观察两次切换：

```text
A → _swtch → B
B → _swtch → A
```

reference 与 exercise 使用相同的结构体布局、checkpoint 和测试线程，便于把观察到的数据直接用于实现。学生先完成观察表，再进入代码补全。

| TODO | 文件 | 任务 |
|---|---|---|
| 1 | `thread.h` | 补全 `thread_context` 的 `ra`、`sp`、`s0-s11` |
| 2 | `swtch.S` | 用旧 `tp` 保存当前 `ra`、`sp`、`s0-s11` |
| 3 | `swtch.S` | 在保存完成后将 `tp` 更新为目标线程 |
| 4 | `swtch.S` | 用新 `tp` 恢复 context 并 `ret` |

TODO 按 context switch 的三个逻辑阶段组织，不预埋错误 offset。未完成时程序停在明确的 `lab16_todo_checkpoint()`，不允许带着不完整 context 继续运行。

框架提供 boot、linker、UART、trap、scheduler、thread allocator、两个已构造 context 的线程以及正确 reference。Lab 16 不处理线程首次启动。

建议结构：

```c
struct thread_context {
    uint64_t ra;
    uint64_t sp;
    uint64_t s[12];
};
```

汇编使用共享 offset 常量 `CTX_RA`、`CTX_SP`、`CTX_S0` 至 `CTX_S11`、`CTX_SIZE`。构建期必须验证 C layout 与汇编常量一致。

## 7. README 概念、执行路径与真实数据

README 应重点说明：

1. thread context 与 trap frame 的区别；
2. RISC-V caller-saved 与 callee-saved registers；
3. cooperative `_swtch()` 为什么保存 `ra`、`sp`、`s0-s11`；
4. `tp` 表示当前线程身份，`sp` 表示当前调用链；
5. context struct、汇编 offset 和实际内存之间的对应；
6. 为什么 `_swtch()` 的 `ret` 可以进入另一个线程；
7. 为什么从每个线程自身视角看，`_swtch()` 像普通函数返回；
8. 为什么 save 与 restore 必须对称。

```text
A calls _swtch(B)
  → use old tp to save A
  → tp = B
  → use new tp to restore B
  → ret uses B.ra
  → B's earlier _swtch call returns
```

正确顺序必须是：

```text
用旧 tp 保存 current
→ 更新 tp
→ 用新 tp 恢复 next
```

README 提供两张待填写的观察表。

### Context switch 阶段

| Checkpoint | `tp` 指向 | `sp` 属于哪个栈 | `ra` 指向 | 当前 context memory |
|---|---|---|---|---|
| A before switch | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| save done | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| `tp` changed | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| restore done | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| B after `ret` | 学生填写 | 学生填写 | 学生填写 | 学生填写 |

### 两个线程的独立状态

| Thread | context address | stack range | saved `sp` | saved `ra` | `s0/s1` sentinels |
|---|---:|---|---:|---:|---|
| A | 学生填写 | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| B | 学生填写 | 学生填写 | 学生填写 | 学生填写 | 学生填写 |

错误 offset 和过早更新 `tp` 不作为必做故障。README 将它们改为推理题：

- 如果 save 与 restore 使用不同 slot，哪个线程的哪个值会在何时发生变化？
- 如果先执行 `tp = next` 再保存，保存操作会写入谁的 context？

教师可将第二题作为可选 Challenge，让学生临时改变一条指令并观察；它不进入必做验收。

## 8. GDB 证据与验收

稳定 checkpoints：

```text
thread_a_before_switch
swtch_save_done
swtch_tp_changed
swtch_restore_done
thread_b_after_switch
thread_a_after_resume
```

学生记录每个 checkpoint 的 `tp`、`sp` 所属栈、`ra`、context memory 和 backtrace。关键证据是：

- save 完成后 A 的 context 与寄存器一致；
- `_swtch()` 可以经历 `tp` 已指向 B、`sp` 暂时仍属于 A 的短暂阶段；
- restore 后 `sp/ra/s0-s11` 都来自 B；
- `ret` 后 B 的旧 `_swtch()` 调用返回；
- B 切回 A 后，A 从原调用点继续。

自动验收：

- C/汇编 layout 一致；
- save/restore offset 对称；
- 更新 `tp` 前已保存当前现场；
- reference 与学生完成版产生相同的语义 switch 序列；
- A/B 严格交替；
- `sp` 始终处于正确 stack range；
- `s0-s11` sentinels 保持；
- 至少完成 100 次 context switch；
- 输出 `LAB16 PASS`。

时间分配：

```text
README：context 与控制流      10–15 分钟
运行 reference 并预测            10 分钟
GDB 观察 A→B→A              20–25 分钟
实现三个 `_swtch` 逻辑块     25–30 分钟
sentinel 与多轮验证              10 分钟
复习与解释                    5–10 分钟
```

# Lab 17：首次启动、Round-Robin 与生命周期

## 9. 核心问题与目标

核心问题：

> 一个从未运行过的线程没有旧现场，内核怎样让它第一次被调度时像普通函数一样开始执行，并在函数返回后安全退出？

预计时间 90–120 分钟。学生应能：

- 为新线程分配独立且 ABI 对齐的栈；
- 构造初始 `sp`、`ra` 和 trampoline 参数；
- 实现 FIFO Round-Robin；
- 跟踪 RUNNING、READY、WAITING、EXITED；
- 实现不返回的 `thread_exit()`；
- 解释 idle thread 的作用；
- 区分主动 `yield` 与阻塞 `condition_wait`；
- 跟踪 wait/broadcast 引起的 WAITING→READY 状态变化；
- 比较 cooperative 与 preemptive scheduling 的控制权和同步影响。

## 10. 观察优先的任务与框架边界

Lab 17 先提供可直接运行的正确 reference。学生在修改代码前，完整观察一次：

```text
main spawn A/B/C
→ A 第一次进入 thread_setup
→ A yield 到 B
→ B/C/main 依次运行
→ C/B/A 先后 return
→ thread_exit 将它们永久移出调度集合
→ main 检查完成
```

观察完成后，exercise 只留下三个局部、语义明确的代码补全点：

| TODO | 文件 | 学生任务 |
|---|---|---|
| 1 | `thread.c` | 根据刚观察的数据补全新线程的 `sp`、`ra`、入口函数、参数和 READY 状态 |
| 2 | `thread.c` | 补全 `thread_yield()` 中“当前线程入队尾、下一线程出队首”的核心操作 |
| 3 | `thread.c` | 补全 `thread_exit()` 的 EXITED 状态和永久切走操作 |

这些 TODO 不通过隐蔽错误制造异常行为。未完成时程序停在带有明确说明的 `lab17_todo_checkpoint()`，避免学生先面对随机跳转、坏栈或损坏队列。

框架提供 Lab 16 的正确 `_swtch()`、正确的只读 `thread_setup.S`、thread entry 分配、stack storage、ready-list primitives、参数搬运、测试线程、main 和 idle 初始结构。学生需要能解释 `thread_setup.S`，但本实验不再要求修改它。

入口统一为：

```c
void thread_function(void *arg);
```

不要求学生处理任意 varargs。

## 11. 首次启动与调度

```text
thread_spawn(fn, arg)
  → allocate entry and stack
  → context.sp = aligned stack top
  → context.ra = thread_setup
  → state = READY
  → insert at ready-list tail

first dispatch
  → _swtch restores artificial context
  → ret to thread_setup
  → call fn(arg)
  → fn returns
  → thread_exit
  → mark EXITED and switch away permanently
```

Round-Robin 规则：

```text
running A, ready [B, C, D]
  → A RUNNING→READY
  → insert A at tail
  → remove B from head
  → B READY→RUNNING
  → _swtch(B)
```

若 ready list 为空，当前非退出线程继续运行；退出线程必须切到普通 READY 线程或 idle。idle 永不退出，只在没有普通 READY 线程时运行。

## 12. README 概念与真实数据观察

README 不以“寻找四个预埋 bug”为主线，而应包含以下内容：

1. program、process、thread 的区别，以及线程共享/私有状态表；
2. 已运行线程 context 与新线程 artificial context 的对照图；
3. `thread_spawn → ready_list → _swtch → thread_setup → worker → thread_exit` 全路径；
4. main、worker、idle 三类线程的差异；
5. RR 队列在每次 yield 前后的状态表；
6. `RUNNING/READY/EXITED` 状态转换图；
7. stack 从高地址向低地址增长和 16-byte ABI 对齐；
8. 为什么 worker 不能直接 return 到未知地址；
9. 为什么 cooperative scheduler 依赖线程主动 yield；
10. cooperative 与 preemptive scheduling 的差异，以及本实验为何不实现 timer preemption；
11. `condition_wait()` 与普通 `thread_yield()` 的区别；
12. 为什么 `condition_wait()` 不能从 ISR 调用，而 `condition_broadcast()` 可以；
13. 学生在每个 GDB checkpoint 应记录什么、数据说明什么。

测试运行三个 worker，迭代次数分别为 3、2、1，并输出结构化事件日志。学生先预测调度顺序，再把真实日志与预测对照。日志记录真实的 TID、`tp`、`sp`、`ra`、state、ready queue head/tail 和 switch count，而不是只打印 `A/B/C`。

README 提供三张待填写的观察表：

### 新线程首次运行

| Checkpoint | TID | `tp` | `sp`/stack range | `ra` | state |
|---|---:|---:|---|---|---|
| created | 学生填写 | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| first dispatch | 学生填写 | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| setup entry | 学生填写 | 学生填写 | 学生填写 | 学生填写 | 学生填写 |

### Round-Robin 队列

| Running | yield 前 ready queue | 当前线程入队后 | selected next | yield 后 ready queue |
|---|---|---|---|---|
| main | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| A | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| B | 学生填写 | 学生填写 | 学生填写 | 学生填写 |

### 线程退出

| Worker | return 前 state | `thread_exit` 后 state | 是否仍在 ready queue | 后续是否再次运行 |
|---|---|---|---|---|
| C | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| B | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| A | 学生填写 | 学生填写 | 学生填写 | 学生填写 |

### Condition wait/broadcast 必做观察

框架提供一个正确、只读的 ECE391 condition 实现。教学路径复用 Week 5 的 UART external interrupt：worker 在 receive buffer 为空时等待 `rxbuf_not_empty`，UART ISR 收到字节后 broadcast。自动验收可通过同一 ISR-facing helper 注入确定性设备事件，避免测试依赖人工输入。

```text
worker running in getchar
→ receive buffer empty
→ condition_wait(rxbuf_not_empty)
→ worker: RUNNING → WAITING
→ worker 进入 condition wait list，并离开 runnable 集合
→ scheduler 运行 main 或 idle
→ UART interrupt receives a byte
→ UART ISR calls condition_broadcast(rxbuf_not_empty)
→ worker: WAITING → READY
→ worker 加入 ready queue
→ scheduler 后续恢复 worker
```

UART ISR 只调用 `condition_broadcast()`，不调用 `condition_wait()`；broadcast 本身不触发立即 yield。自动测试的事件注入必须走与 UART ISR 相同的 broadcast helper，不允许直接修改 worker state 或 ready queue。

学生填写：

| Checkpoint | running | worker state | ready queue | condition wait list |
|---|---|---|---|---|
| before wait | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| after sleep | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| after broadcast | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| after resume | 学生填写 | 学生填写 | 学生填写 | 学生填写 |

学生必须解释：

- `thread_yield()` 后当前线程仍是 READY，`condition_wait()` 后当前线程是 WAITING；
- WAITING 线程不能留在 ready queue；
- broadcast 只把 waiter 变为 READY，不保证它立即运行；
- `condition_wait()` 可能在 interrupts disabled 时进入；scheduler 在运行其他线程期间按约定处理 interrupt state，并在 wait 返回前恢复调用环境需要的状态；
- condition 实现必须避免“条件已 broadcast，但线程尚未进入 wait list”的 lost wakeup 窗口。

本段只要求观察和解释，不要求学生实现 condition 内部队列。完整实现仍属于 Challenge。

## 13. GDB 证据与验收

稳定 checkpoints：

```text
thread_created
thread_first_dispatch
thread_setup_entry
scheduler_before_switch
scheduler_after_select
thread_function_returned
thread_marked_exited
condition_wait_entered
condition_thread_sleeping
condition_broadcast_done
condition_thread_resumed
```

学生验证：

- 新线程 `sp` 在自己的 stack range 且 16-byte aligned；
- 初始 `ra == thread_setup`；
- 新线程创建后为 READY；
- ready queue 始终遵循 FIFO；
- 每个线程首次入口和 exit 各发生一次。
- condition waiter 经历 RUNNING→WAITING→READY→RUNNING；
- WAITING 期间 worker 不在 ready queue；
- broadcast 后 worker 进入 ready queue，但不要求立即运行。

自动验收：

- 至少 20 次合计切换；
- reference 与学生完成版产生相同的语义事件和状态转换序列；地址值只检查所属对象或 stack range，不要求数值完全相同；
- RR 事件日志顺序正确；
- 每个线程只使用自己的栈；
- EXITED 线程不再调度；
- condition wait list 和 ready queue 中的成员关系正确；
- main 和 idle 生命周期合法；
- 输出 `LAB17 PASS`。

时间分配：

```text
README 概念与流程预测         10–15 分钟
观察首次启动和 RR             20–25 分钟
观察 condition 状态流         15–20 分钟
补全三个核心代码点            20–25 分钟
观察退出并完成验收            10–15 分钟
复习题                         5–10 分钟
```

# Lab 18：Race、Lock 与 Interrupt Critical Section

## 14. 核心问题与目标

核心问题：

> 两个线程都正确执行自己的 C 代码，为什么共享结果仍然会错？锁与关闭中断分别阻止了谁？

预计时间 90–120 分钟。必做部分分为：

1. 观察未同步线程的确定性 lost update；
2. 对照观察正确阻塞式 lock 如何改变线程状态和调度；
3. 对照观察 interrupt masking 如何保护 thread-ISR 共享状态。

学生应能识别共享不变量、确定最小正确临界区、解释 cooperative race、避免持锁 busy-spin，并区分 lock 与 interrupt masking。

## 15. Part A：确定性 Lost Update

两个 worker 各执行 `N` 次：

```c
uint64_t value = shared_counter;
thread_yield();
shared_counter = value + 1;
```

`N=3` 时，A/B 在每次 load 与 store 间交错，期望值为 6，实际值稳定为 3。显式 yield 用于把合法 interleaving 变成可重复实验，不代表真实 race 必须显式 yield。

未同步基线由框架完整提供，不要求学生先修复代码。学生任务是收集证据：

| 观察任务 | 文件 | 任务 |
|---|---|---|
| 1 | README worksheet | 标出共享数据、线程私有数据和期望不变量 |
| 2 | GDB/事件日志 | 记录 load/store、thread ID、local value 和 shared value |
| 3 | README worksheet | 写出导致 lost update 的完整 interleaving |

稳定 checkpoints 为 `counter_loaded`、`counter_before_store`、`counter_stored`、`counter_final_check`。学生必须证明哪个 store 覆盖了哪个更新，再进入同步机制。

## 16. Part B：阻塞式 Lock

最小接口：

```c
struct lock {
    struct thread *owner;
    struct condition released;
};

void lock_init(struct lock *lock);
void lock_acquire(struct lock *lock);
void lock_release(struct lock *lock);
```

必做路径由框架提供已验证的 `lock_init()`、`condition_wait()`、`condition_broadcast()` 和 owner-violation checkpoint。学生阅读这些代码，并在 Part B 调用 condition helper，但不实现 condition 内部队列。Condition Variable Challenge 才要求学生补全 condition 的核心状态转换。因此必做 lock 不依赖学生先完成挑战题。

Part B 先运行正确 lock reference。即使 A 在临界区内 yield，B 也不能进入同一临界区：

```text
A acquire
→ A enters critical section
→ A yields while holding lock
→ B tries acquire
→ B becomes WAITING
→ A resumes and releases
→ B becomes READY
→ B resumes, rechecks and acquires
```

学生先填写真实状态表：

| Checkpoint | running | lock owner | B state | ready queue | counter |
|---|---|---|---|---|---:|
| A acquired | 学生填写 | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| B waits | 学生填写 | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| A releases | 学生填写 | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| B acquired | 学生填写 | 学生填写 | 学生填写 | 学生填写 | 学生填写 |

观察后再补全少量关键代码：

| TODO | 文件 | 任务 |
|---|---|---|
| 1 | `lock.c` | 用 `while`、框架 condition helper 和 owner 补全 acquire |
| 2 | `lock.c` | 清除 owner、broadcast 并补全 release |
| 3 | `race.c` | 用一个临界区覆盖完整 read-modify-write |

错误修复只分别锁住 load 和 store，仍会 lost update。正确临界区必须覆盖完整不变量：

```c
lock_acquire(&counter_lock);
value = shared_counter;
thread_yield();
shared_counter = value + 1;
lock_release(&counter_lock);
```

保留临界区内 yield，用来证明正确性来自 mutual exclusion。

等待条件必须使用 `while`。broadcast 只说明状态可能改变；被唤醒线程实际恢复时，其他线程可能已重新持锁。

本实验不预埋临界区过小、`if` 代替 `while`、non-owner release 或 busy-spin 等错误。这些反例放在 README 中作为代码片段和预测题，并由自动测试检查最终实现的安全性质。教师可选择其中一个作为额外 Challenge。

## 17. Part C：线程与 ISR

共享状态：

```c
volatile uint64_t event_count;
volatile uint64_t event_checksum;
```

不变量：

```text
event_checksum == checksum(event_count)
```

`event_checksum` 只是让 thread-ISR interleaving 可观察的测试不变量，不是同步机制，也不是学生需要掌握的 ECE391 数据结构。真正的学习目标是识别成组共享状态，并保证 ISR 不能观察到中间状态。

普通 thread lock 不能自动阻止 ISR。ISR 若获取一个被被中断线程持有的阻塞锁，会等待无法恢复的 owner，形成死锁。

Part C 先运行未保护基线：

```text
thread updates event_count
→ interrupt arrives before checksum update
→ ISR reads the pair
→ ISR observes a broken invariant
```

再运行正确 reference：

```text
save old SIE
→ clear SIE
→ update both fields
→ restore old SIE
→ pending ISR executes
→ ISR observes a consistent pair
```

学生记录：

| Checkpoint | `sstatus.SIE` | interrupt pending | `event_count` | `event_checksum` | invariant |
|---|---:|---:|---:|---:|---|
| before critical section | 学生填写 | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| after SIE clear | 学生填写 | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| between two updates | 学生填写 | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| after restore | 学生填写 | 学生填写 | 学生填写 | 学生填写 | 学生填写 |
| ISR entry | 学生填写 | 学生填写 | 学生填写 | 学生填写 | 学生填写 |

接口：

```c
typedef uint64_t irq_state_t;
irq_state_t irq_save_disable(void);
void irq_restore(irq_state_t previous);
```

学生 TODO：

| TODO | 文件 | 任务 |
|---|---|---|
| 4 | `irq.c` | 原子读取并清除 `sstatus.SIE` |
| 5 | `irq.c` | 按保存值恢复原 interrupt state |
| 6 | `event.c` | 用一个 interrupt critical section 保护两个共享字段 |

未保护基线会在两个字段更新之间安排 pending interrupt，这是需要观察的真实 thread-ISR interleaving，不是排错任务。退出时无条件 enable、遗漏恢复和 ISR 获取阻塞锁改为 README 推理题，不作为预埋故障。

## 18. Lab 18 验收

Part A：

- unsafe 版本稳定重现 lost update；
- 日志包含两个线程读取同一旧值；
- 学生提交准确 interleaving。

Part B：

- 2–3 个线程各执行至少 1,000 次更新；
- 临界区内保留 yield；
- 同时进入临界区的线程数永不超过 1；
- non-owner release 被拒绝；
- 无 starvation 或 hang；
- counter 精确等于期望值；
- reference 与学生完成版产生相同的 owner/waiter 状态转换。

Part C：

- ISR 不观察到不一致的成对字段；
- SIE 初始开启时正确恢复为开启；
- SIE 初始关闭时正确恢复为关闭；
- nested save/restore 顺序正确；
- 所有退出路径恢复旧状态；
- reference 与学生完成版产生相同的 interrupt-state 转换。

最终输出：

```text
[Part A] deterministic race observed
[Part B] lock preserves counter
[Part B] mutual exclusion verified
[Part C] ISR invariant preserved
[Part C] interrupt state restored
LAB18 PASS
```

时间分配：

```text
README：共享状态与不变量         10–15 分钟
Part A 观察并解释 race           15–20 分钟
Part B 观察 lock 状态流          15–20 分钟
Part B 补全并验证 lock           20–25 分钟
Part C 对照观察并补全 IRQ        20–25 分钟
复习题                           5–10 分钟
```

## 19. Condition Variable 实现 Challenge

Challenge 使用与讲义一致的接口：

```c
void condition_init(struct condition *cond, const char *name);
void condition_wait(struct condition *cond);
void condition_broadcast(struct condition *cond);
```

学生补全：

```text
condition_wait(cond):
  保存并关闭 interrupt，保护 wait list 与 scheduler state
  将 current 加入 cond wait list
  current → WAITING
  调用 scheduler 切换到其他 runnable thread
  scheduler 在运行其他线程期间按约定处理 interrupt enable
  被 broadcast 唤醒
  恢复调用 condition_wait 前所要求的 interrupt state
  返回，由调用者的 while 重新检查设备条件

condition_broadcast(cond):
  将 wait list 中所有线程设为 READY
  将它们加入 ready queue
  不主动 yield
```

Lab 17 已要求通过 UART 路径观察和解释状态流。本 Challenge 进一步要求学生实现 ECE391 风格的 wait list、interrupt discipline 和状态转换，重点是避免“设备条件检查后、线程真正进入 wait list 前”发生 broadcast 所造成的 lost wakeup。预计额外 30–45 分钟。

## 20. 三实验能力闭环

```text
Lab 16：保存和恢复 CPU context
    ↓
理解线程切换的机器级机制
    ↓
Lab 17：构造初始 context、ready queue 和 lifecycle
    ↓
建立最小 cooperative threading system
    ↓
Lab 18：race、lock 和 interrupt exclusion
    ↓
维护并发内核中的共享状态
```

完成后，学生既能阅读 ECE391 线程框架，也能独立编写最小 cooperative context switch、线程创建/调度/退出、基本 lock 和 interrupt critical section，并能用 GDB 从机器状态证明实现正确。
