# Lab 15 完整 Trap Frame 与 UART External Interrupt 教学设计

## 1. 定位

Lab 15 是 Lab 14 的直接后续实验，预计用时 1–2 小时。它将两个紧密相关的 ECE391 教学主题合并：

1. 完整保存和恢复 trap frame；
2. 通过 PLIC 处理真实 UART external interrupt。

本实验对应本地课程讲义：

- ECE391-L5：hardware interrupt、CSR、PLIC、UART interrupt；
- ECE391-L6：trap handler、完整 trap frame、`scause` 分发、PLIC claim/complete。

本实验不引入 thread、scheduler、context switch、虚拟内存或完整 device-driver abstraction。

## 2. 与 Lab 14 的边界

Lab 14 已经完成：

- M-mode boot、PMP 与 M→S；
- S-mode kernel、ELF loader 与 S→U；
- `stvec`、`sepc`、`scause`、`sstatus`、`sscratch`；
- user/kernel stack 交换；
- 受控 `ecall`；
- 只适用于固定用户程序的最小寄存器保存区。

Lab 15 复用这些框架，不再要求学生修改 boot 或 ELF loader。Lab 15 解决 Lab 14 留下的问题：

> 如果 UART interrupt 能在任意用户指令处发生，kernel 如何保证 `sret` 后用户程序的全部寄存器保持不变？

## 3. 学习目标

完成实验后，学生应能够：

1. 解释 trap frame 与普通 C stack frame 的区别；
2. 解释异步 interrupt 为什么要求保存完整通用寄存器状态；
3. 根据汇编 offset 与 `struct trap_frame` 字段建立一一对应关系；
4. 解释 trap entry、C dispatcher 和 device ISR 的职责边界；
5. 根据 `scause` 的 interrupt bit 和 cause code 区分 `ecall` 与 supervisor external interrupt；
6. 解释 UART、PLIC 和 CPU 三层 interrupt enable；
7. 完成 PLIC claim → ISR → complete；
8. 使用 GDB 验证 trap frame、`scause`、PLIC source ID 和寄存器恢复结果。

## 4. 总体运行流程

```text
Lab 14 framework
  -> enter U-mode
  -> user program establishes register sentinels

Part A: controlled ecall
  -> user executes a no-op register-check ecall
  -> trap.S saves complete trap frame
  -> C trap dispatcher handles ecall
  -> trap.S restores complete trap frame
  -> sret
  -> user program verifies sentinels

Part B: UART input
  -> UART raises source 10
  -> PLIC reports supervisor external interrupt
  -> CPU jumps to stvec
  -> trap.S saves complete trap frame
  -> C dispatcher recognizes external interrupt
  -> PLIC claim
  -> UART receive ISR reads one byte
  -> PLIC complete
  -> trap.S restores complete trap frame
  -> sret
  -> user program verifies sentinels again
```

Part A 使用不会改变用户通用寄存器的专用 register-check ecall。它只推进 `sepc`，不设置 syscall 返回值。Part A 必须先通过，再启用 Part B。这样学生可以先独立验证 trap frame，避免把寄存器保存错误与 PLIC 配置错误混在一起。

## 5. Trap Frame 设计

`trap_frame` 保存以下状态。这里的“全部通用寄存器”指除恒为零的 `x0` 以外的所有通用寄存器：

```text
ra
gp
tp
t0-t6
s0-s11
a0-a7
original sp
sepc
sstatus
scause
```

不保存 `x0`，因为它恒为零。`sp` 字段保存 trap 发生时的 user stack pointer；trap handler 自身使用 kernel stack。进入 trap 后，`csrrw` 已把原 user `sp` 放入 `sscratch`，trap entry 从 `sscratch` 取得该值并写入 frame。

汇编与 C 共享同一组明确的 offset 常量。学生不自行计算 magic numbers。README 提供字段表和内存布局图。

Lab 15 不支持 nested trap。进入 handler 后保持 S-mode interrupt disabled，恢复现场并执行 `sret` 时再按硬件规则恢复 interrupt enable 状态。

## 6. Trap Entry 与 C Dispatcher

### 6.1 `trap.S`

负责：

- 使用 `sscratch` 切换 user/kernel stack；
- 分配 trap frame；
- 保存完整寄存器和 CSR 状态；
- 将 `struct trap_frame *` 作为参数传给 C；
- 从 trap frame 恢复状态；
- 换回 user stack；
- 执行 `sret`。

### 6.2 `trap.c`

负责：

- 读取 trap frame 中的 `scause`；
- 区分 exception 与 interrupt；
- 保留 Lab 14 的 user `ecall` 路径；
- 将 supervisor external interrupt 交给 PLIC dispatcher；
- 对未知 cause 进入稳定 failure checkpoint。

### 6.3 UART ISR

负责：

- 读取 UART interrupt/status 信息；
- 从 receive buffer register 读取一个输入字节；
- 保存最后收到的字节或写入框架提供的单字节状态；
- 清除设备侧的 interrupt condition。

本实验不要求实现 ring buffer、阻塞 I/O 或通用 device-driver interface。

## 7. Interrupt 配置

框架提供地址、bit mask 和 helper，学生只补关键调用或关键 CSR 操作。

### 7.1 M-mode framework

框架完成：

- 将 supervisor external interrupt 委托给 S-mode；
- 保留 Lab 14 的 exception delegation；
- 配置 PMP。

学生不修改 `boot.S`。

这里的 delegation 具体是设置 `mideleg` 中的 supervisor external interrupt 位；它与 Lab 14 用 `medeleg` 委托 U-mode `ecall` 是两件不同的事。

### 7.2 UART

框架完成 UART 基本初始化。学生启用 receive-data interrupt。

### 7.3 PLIC

框架提供 PLIC MMIO helper。学生完成：

- 为 UART source 10 设置非零 priority；
- 在 hart 0 S-mode context 1 中 enable source 10；
- 设置低于 UART priority 的 threshold。

### 7.4 CPU

学生完成：

- 设置 `sie.SEIE`；
- 设置 `sstatus.SIE`；
- 保证 `stvec` 在启用 interrupt 之前已经有效。

## 8. 学生 TODO

```text
TODO 1  根据给定布局保存完整 trap frame
TODO 2  将 trap_frame 指针传给 C trap dispatcher
TODO 3  根据 scause 区分 user ecall 与 supervisor external interrupt
TODO 4  配置 UART、PLIC 和 CPU 的 external interrupt enable
TODO 5  执行 PLIC claim → UART ISR → PLIC complete
TODO 6  恢复完整 trap frame 并执行 sret
```

TODO 1 和 TODO 6 提供完整寄存器清单与 offset，学生只需要补全对应 `sd`/`ld`。PLIC 地址计算、UART polling 输出和 ELF loader 完整提供。

## 9. 文件结构

```text
week5-csr-interrupt-trap/
  lab15-trap-frame-uart-interrupt/
    README.md
    Makefile
    boot.S
    enter_user.S
    trap.S
    trap_frame.h
    trap.c
    plic.c
    plic.h
    uart.c
    uart.h
    kernel.c
    elf_loader.c
    elf_loader.h
    user/
      interrupt_probe.S
    kernel.ld
    user.ld
    user_image.S
    check-lab15.sh
    run-check-lab15.sh
```

Lab 15 可以从 Lab 14 复制最小框架，但 README 必须把“沿用框架”和“新增学生任务”清楚分开。

## 10. Guided Mode

### Part A：Trap Frame

1. 在 trap entry 前检查 user register sentinels；
2. 在保存完成 checkpoint 查看 `struct trap_frame`；
3. 对照汇编 offset 与 C 字段；
4. 返回 U-mode 后验证所有 sentinels；
5. 修复保存或恢复不对称的问题。

### Part B：UART External Interrupt

1. 检查 `stvec` 已设置；
2. 检查 UART IER、PLIC priority/enable/threshold；
3. 检查 `sie.SEIE` 与 `sstatus.SIE`；
4. 在输入一个字符后停在 trap entry；
5. 检查 `scause` interrupt bit 和 cause code 9；
6. 检查 PLIC claim 返回 source 10；
7. 单步 UART ISR 和 PLIC complete；
8. 返回 U-mode 后再次验证 register sentinels。

对 `sret` 等跨 privilege 指令不要求使用 `stepi`；使用目标地址临时断点加 `continue`，避免部分 QEMU/GDB 版本的 remote single-step 问题。

## 11. 故障设计

exercise mode 保留确定性错误：

- 少保存一个寄存器，用户 sentinel 检查失败；
- 未设置 `sie.SEIE`，UART 有 pending 但 CPU 不进入 trap；
- 忘记 PLIC complete，interrupt 重复出现；
- restore offset 错误，返回后寄存器不匹配。

每个错误都必须有稳定 checkpoint，不依赖随机时序或不可重复 crash。

## 12. 自动验证

### 静态检查

- `trap_frame` 大小和 offset 与汇编常量一致；
- kernel/user ELF 含有所需 checkpoint symbols；
- user ELF entry 和 `PT_LOAD` 合法；
- solution/exercise 使用独立 build 目录。

### QEMU solution smoke test

自动测试分两层：

1. 无交互 Part A：确认完整 trap frame 往返和 sentinel 检查通过；
2. Part B：优先通过 QEMU monitor/serial input 注入一个字符；若环境不能可靠注入，则保留为人工 GDB walkthrough，并用可控的框架触发模式验证 dispatcher。

成功输出至少包括：

```text
[S] full trap frame ok
[S] UART interrupt: source 10
[S] received: <character>
LAB15 PASS
```

## 13. 时间预算

```text
Concept Warm-up               15–20 min
Part A trap frame             35–45 min
Part B UART/PLIC interrupt    35–45 min
GDB verification             15–20 min
```

为维持 1–2 小时，ring buffer、通用 ISR registration table、device interface、nested interrupt 和 thread context 均不作为学生任务。

## 14. 成功标准

学生最终能够用一条完整因果链解释：

```text
UART 为什么产生 interrupt
→ PLIC 为什么报告 source 10
→ CPU 为什么跳到 stvec
→ trap frame 为什么必须完整
→ handler 为什么必须 claim 和 complete
→ sret 后用户寄存器为什么仍保持原值
```
