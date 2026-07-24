# Lab 14：从 M-mode 进入用户程序，再通过 `ecall` 回到内核

## 这个实验在做什么？

前面的实验已经练习了：

```text
QEMU 启动
  -> GDB remote connection
  -> breakpoint
  -> pc、寄存器和当前指令
  -> exception 前的状态
```

Lab 14 将这些工具连接成一条 ECE391-style OS 控制流：

```text
M-mode boot
  -> S-mode kernel
  -> load an independent ELF user program
  -> sret
  -> U-mode program
  -> ecall
  -> S-mode trap handler
  -> sret
  -> U-mode program
  -> exit ecall
  -> S-mode kernel
```

本实验不要求你从零实现 ELF loader，也不要求实现完整 trap frame。你需要理解并验证每一次 PC 和 privilege mode 为什么改变。

预计时间：约 105 分钟。

---

## 完成实验后，你应该能够

1. 解释 CSR 和普通寄存器的区别；
2. 解释 `mstatus`、`mepc`、`sstatus`、`sepc`、`stvec`、`scause` 和 `sscratch`；
3. 根据 `MPP/mepc` 预测 `mret` 的结果；
4. 根据 `SPP/sepc` 预测 `sret` 的结果；
5. 解释 U-mode `ecall` 时 CPU 自动完成什么；
6. 解释 trap handler 还必须完成什么；
7. 使用 GDB 证明 M→S→U→S→U→S；
8. 解释为什么返回前必须执行 `sepc += 4`；
9. 解释本实验的最小保存区为什么不是完整 trap frame。

---

# Concept Warm-up

先完成这一部分，再运行程序。目标不是背诵 CSR，而是建立一个可以预测 CPU 行为的模型。

## 1. 什么是 privilege mode？

RISC-V 定义不同的 privilege mode。本实验使用三个：

| Mode | 本实验中的工作 |
|---|---|
| M-mode | CPU 初始 boot，建立 S-mode 环境 |
| S-mode | 运行 kernel、加载 user ELF、处理 trap |
| U-mode | 运行用户程序 |

最重要的理解是：

> Privilege mode 是 CPU 当前状态，不是某个函数或某段内存永久具有的属性。

假设 `pc` 指向同一条 CSR 指令：

- CPU 处于 S-mode 时，它可能合法；
- CPU 处于 U-mode 时，它可能产生 illegal-instruction exception。

普通的 `j`、`call` 和 `ret` 只改变 PC，不会自动改变 privilege mode。

## 2. 什么是 CSR？

CSR 是 **Control and Status Register**，即控制与状态寄存器。

```text
普通寄存器 x0-x31
    保存程序正在计算的数据、地址、参数和返回值

CSR
    保存或控制 CPU 自身的运行状态
```

例如：

```text
a0    普通寄存器，可保存函数参数或 syscall 返回值
sepc  CSR，保存 S-mode trap 相关的 PC
```

看到一个 CSR 时，不要先背名字。依次问：

```text
1. 它记录或控制什么？
2. 谁更新它：CPU 硬件还是 kernel？
3. 哪个事件或返回指令使用它？
```

## 3. 本实验使用的重要 CSR

| CSR | 英文全称与缩写拆解 | 它回答的问题 | 谁写入 | 谁使用 |
|---|---|---|---|---|
| `mstatus.MPP` | **M**achine **Status** Register / **M**achine **P**revious **P**rivilege mode | `mret` 后进入哪个 mode？ | M-mode kernel | `mret` |
| `mepc` | **M**achine **E**xception **P**rogram **C**ounter | `mret` 后从哪个 PC 执行？ | M-mode kernel 或 trap 硬件 | `mret` |
| `medeleg` | **M**achine **E**xception **Deleg**ation Register | 哪些 exception 交给 S-mode？ | M-mode kernel | trap 硬件 |
| `sstatus.SPP` | **S**upervisor **Status** Register / **S**upervisor **P**revious **P**rivilege mode | trap 前来自哪个 mode？`sret` 返回哪个 mode？ | trap 硬件；kernel 也可修改 | `sret` |
| `sepc` | **S**upervisor **E**xception **P**rogram **C**ounter | trap 相关的 PC 是多少？ | trap 硬件；handler 也可修改 | `sret` |
| `stvec` | **S**upervisor **T**rap **Vec**tor Base Address Register | S-mode trap handler 在哪里？ | S-mode kernel | trap 硬件 |
| `scause` | **S**upervisor **Cause** Register | 为什么发生 trap？ | trap 硬件 | trap handler |
| `sscratch` | **S**upervisor **Scratch** Register | trap entry 从哪里取得 kernel stack？ | kernel 和 trap entry | trap entry |
| `satp` | **S**upervisor **A**ddress **T**ranslation and **P**rotection Register | 是否启用地址转换？ | S-mode kernel | 地址转换硬件 |

这些名字有稳定的助记规律：

```text
m       Machine
s       Supervisor
status  CPU status
epc     Exception Program Counter
tvec    Trap Vector
cause   trap cause
scratch 临时保存位置
atp     Address Translation and Protection
```

例如，看到 `sepc` 时可以先拆成：

```text
s + epc
Supervisor + Exception Program Counter
```

因此即使暂时忘记细节，也能推断它与 S-mode trap 的程序计数器有关。

本实验设置 `satp = 0`，不启用页表。

## 4. 怎样访问 CSR？

CSR 不能通过普通 load/store 指令直接访问。本实验只需要记住四个英文字母：

```text
csr  Control and Status Register
r    Read
w    Write
s    Set bits
c    Clear bits
```

因此可以这样读命令：

| 汇编 | 英文助记 | 简单含义 |
|---|---|---|
| `csrr t0, sepc` | CSR **R**ead | 读取：`t0 = sepc` |
| `csrw sepc, t0` | CSR **W**rite | 写入：`sepc = t0` |
| `csrs sstatus, t0` | CSR **S**et bits | `t0` 中为 1 的 bits 在 `sstatus` 中被设为 1 |
| `csrc sstatus, t0` | CSR **C**lear bits | `t0` 中为 1 的 bits 在 `sstatus` 中被清为 0 |
| `csrrw t0, sscratch, t1` | CSR **R**ead and **W**rite | 读出旧 `sscratch` 到 `t0`，再把 `t1` 写入 `sscratch` |

### 4.1 怎样记住操作数顺序？

RISC-V 常用的操作数习惯是：

```text
目标在前，来源在后
destination, source
```

对于 `csrrw`，CSR 固定放在中间：

```asm
csrrw rd, csr, rs
      目标  CSR  来源
```

可以记成：

> 旧 CSR 向左读，新值从右写。

```text
rd  ←  csr  ←  rs
```

具体例子：

```asm
csrrw t0, sscratch, t1
```

假设执行前：

```text
sscratch = 100
t1       = 200
```

执行后：

```text
t0       = 100    旧 CSR 值被读到目标寄存器
sscratch = 200    来源寄存器的值被写入 CSR
```

本实验使用：

```asm
csrrw sp, sscratch, sp
```

左右两边恰好都是 `sp`，因此：

```text
sp       ← 旧 sscratch
sscratch ← 旧 sp
```

它会交换 user stack pointer 和 kernel stack pointer。

`csrr`、`csrw`、`csrs` 和 `csrc` 是 assembler 提供的方便写法。本实验直接使用即可，不展开它们的底层编码形式。

设置单个字段时，通常不要把整个 CSR 粗暴覆盖成一个常量。应通过 set/clear 只改变目标 bits。

---

## 5. 具体例子：从 M-mode 进入 S-mode

假设：

```text
supervisor_entry = 0x80200000
current mode     = M
```

### 5.1 设置 `mepc`

```asm
la   t0, supervisor_entry
csrw mepc, t0
```

执行后：

```text
t0   = 0x80200000
mepc = 0x80200000
```

### 5.2 设置 `mstatus.MPP`

`MPP` 位于 `mstatus[12:11]`：

```text
MPP = 01  表示 S-mode
```

```asm
li   t0, (3 << 11)
csrc mstatus, t0       # 清除旧 MPP
li   t0, (1 << 11)
csrs mstatus, t0       # 设置 MPP = S
```

### 5.3 执行 `mret`

```asm
mret
```

CPU 使用：

```text
pc   <- mepc
mode <- mstatus.MPP
```

结果：

```text
pc           = 0x80200000
current mode = S
```

`mret` 不是跳到 `t0`。`t0` 只是帮助 kernel 设置 `mepc`，真正被 `mret` 读取的是 CSR。

---

## 6. 具体例子：加载 ELF 并进入 U-mode

假设 loader 给出：

```text
embedded_user_elf_start = 0x800xxxxx
loaded PT_LOAD          = 0x80400000 .. 0x80400fff
ELF e_entry             = 0x80400000
user stack top          = 0x80410000
kernel stack top        = 0x800xxxxx
```

注意三个地址的区别：

```text
embedded_user_elf_start  ELF 文件字节在 kernel image 中的位置
PT_LOAD destination      用户代码实际被复制到的位置
e_entry                  用户程序第一条指令的位置
```

进入用户程序时，`sepc` 必须使用 `e_entry`，不能使用 embedded ELF 的源地址。

```asm
li   t0, 0x80400000
csrw sepc, t0

li   t0, (1 << 8)       # SSTATUS_SPP
csrc sstatus, t0        # SPP = 0，选择 U-mode

csrw sscratch, sp       # 保存 kernel sp
li   sp, 0x80410000     # 安装 user sp
sret
```

`sret` 使用：

```text
pc   <- sepc
mode <- sstatus.SPP
```

结果：

```text
pc           = 0x80400000
current mode = U
sp           = 0x80410000
```

---

## 7. 具体例子：U-mode 执行 `ecall`

用户程序：

```asm
li a7, 1               # SYS_PROBE
ecall                   # 本实验中地址为 0x80400004
probe_returned:
li t0, 0x391
bne a0, t0, user_fail
```

执行 `ecall` 前：

```text
mode     = U
pc       = 0x80400004
a7       = 1
sp       = 0x80410000
sscratch = kernel stack
```

因为 M-mode boot 已将 U-mode `ecall` 委托给 S-mode，CPU 自动执行：

```text
sepc        <- 0x80400004
scause      <- 8
sstatus.SPP <- 0
pc          <- stvec
mode        <- S
```

这里：

```text
scause = 8  environment call from U-mode
SPP = 0     trap 之前处于 U-mode
```

### CPU 不会自动做什么？

硬件不会：

```text
保存全部普通寄存器
切换到 kernel stack
检查 a7 中的 syscall number
让 sepc 自动跳过 ecall
设置 a0 返回值
```

这些工作必须由 trap handler 完成。

---

## 8. 具体例子：交换 user stack 和 kernel stack

trap entry 开始时：

```text
sp       = 0x80410000    user stack
sscratch = 0x800xxxxx    kernel stack
```

执行：

```asm
csrrw sp, sscratch, sp
```

之后：

```text
sp       = 0x800xxxxx    kernel stack
sscratch = 0x80410000    暂存 user stack
```

返回前再次执行相同指令，交换回用户栈。

这说明：

> Trap 改变 privilege mode 和 PC，但不会自动改变 `sp`。

---

## 9. 为什么必须执行 `sepc += 4`？

`ecall` 是同步 exception，`sepc` 指向触发 trap 的 `ecall`：

```text
ecall address = 0x80400004
sepc          = 0x80400004
```

如果 handler 直接 `sret`：

```text
sret
  -> pc = 0x80400004
  -> 再次执行 ecall
  -> 再次 trap
  -> 无限重复
```

正确处理：

```asm
csrr t0, sepc
addi t0, t0, 4
csrw sepc, t0
sret
```

结果：

```text
pc   = 0x80400008
mode = U
a0   = 0x391
```

本实验的 `ecall` 是固定 32-bit 指令，所以加 4。不要把“所有 exception 都给 `sepc` 加 4”当成通用规则。

---

## 10. 什么是 exception、interrupt 和 trap？

```text
exception
    与当前执行指令相关的同步事件
    本实验的 ecall 是 exception

interrupt
    外部或内部设备产生的异步事件
    将在后续 timer、UART/PLIC 实验中学习

trap
    exception 或 interrupt 导致控制转移到 handler 的过程
```

本实验只制造同步、可重复的 `ecall` trap。异步 interrupt 可能发生在任意用户指令上，因此需要完整 trap frame。

---

# 实验文件

```text
boot.S          M-mode boot、PMP、delegation、M→S
kernel.c        S-mode kernel、安装 stvec、调用 ELF loader
elf_loader.c    已完成的最小 ELF64 PT_LOAD loader
enter_user.S    设置 sepc/SPP/stack，S→U
trap.S          stack 交换、最小保存、probe/exit
user/probe.S    独立 user ELF
kernel.ld       kernel 链接布局
user.ld         user ELF 链接到 0x80400000
```

源码中有五个 `LAB14 TODO` 教学点，并提供两个构建模式：

```text
MODE=solution   完整参考实现，用于先建立正常基线
MODE=exercise   可构建但行为故意错误，学生逐项补全五个 TODO
```

两个模式使用不同构建目录，避免旧 object file 混入：

```text
build/solution/
build/exercise/
```

---

# 构建和运行

检查工具：

```bash
make check-tools
```

构建：

```bash
make MODE=solution
```

查看独立 user ELF：

```bash
make MODE=solution user-info
```

重点观察：

```text
Class:        ELF64
Machine:      RISC-V
Entry point:  0x80400000
Program Headers 中存在 LOAD
```

运行：

```bash
make MODE=solution run
```

预期输出：

```text
[M] boot
[S] kernel
[S] user ELF loaded
[U] probe returned
[S] user exit: 0
LAB14 PASS
```

`[U]` 表示该结果由 U-mode 执行路径产生，实际文字由 kernel 在收到 `SYS_EXIT` 后通过 UART 输出。当前 `satp=0` 且 PMP 放行，不代表已经建立安全的用户/内核内存隔离。

退出 QEMU：

```text
Ctrl-a x
```

建立正常基线后，构建练习版本：

```bash
make MODE=exercise
make MODE=exercise debug
```

`exercise` 版本故意保留以下错误：

```text
TODO 1  没有设置 mstatus.MPP
TODO 2  没有设置 mepc
TODO 3  stvec 被设置为 0
TODO 4  sepc/SPP 未正确准备
TODO 5  没有跳过 ecall，也没有返回 0x391
```

从 TODO 1 和 TODO 2 开始修复。每修复一个阶段，都先在对应 checkpoint 用 GDB 验证 CSR，再继续下一阶段。

---

# 运行前预测表

先填写“你的预测”，再启动 GDB：

| Checkpoint | 当前 mode | 下一 PC 来自哪里 | 下一 mode 由什么决定 |
|---|---|---|---|
| `before_mret` |  |  |  |
| `before_user_sret` |  |  |  |
| `user_probe_ecall` 之后 |  |  |  |
| `before_probe_sret` |  |  |  |

参考模型应为：

| Checkpoint | 当前 mode | 下一 PC 来源 | 下一 mode 的依据 |
|---|---|---|---|
| `before_mret` | M | `mepc` | `mstatus.MPP` |
| `before_user_sret` | S | `sepc` | `sstatus.SPP` |
| `user_probe_ecall` 之后 | S | `stvec` | trap 硬件规则 |
| `before_probe_sret` | S | `sepc` | `sstatus.SPP` |

---

# Guided Mode

## Step 1：启动等待 GDB 的 QEMU

terminal 1：

```bash
make MODE=solution debug
```

terminal 2：

```bash
make MODE=solution gdb
```

`make gdb` 已经加载 kernel symbols 并连接 `:1234`。

## Step 2：观察 M-mode 返回配置

```gdb
break before_mret
continue
info registers pc mstatus mepc medeleg
x/i $pc
```

要回答：

```text
mepc 是否指向 supervisor_entry？
MPP 是否选择 S-mode？
medeleg 的 bit 8 是否设置？
```

单步执行：

```gdb
stepi
info registers pc
```

`pc` 应进入 `supervisor_entry`。

## Step 3：观察真实 ELF load

```gdb
break elf_load_done
continue
p/x loaded_user.entry
p/x loaded_user.segment_start
p/x loaded_user.segment_end
x/8i loaded_user.entry
```

要回答：

```text
ELF entry 是多少？
PT_LOAD 被复制到哪个范围？
entry 处是否能反汇编出 user/probe.S？
```

## Step 4：观察 S-mode 进入 U-mode 的准备

```gdb
break before_user_sret
continue
info registers pc sstatus sepc sp sscratch stvec
x/8i $sepc
```

要回答：

```text
sepc 是否等于 ELF entry？
SPP 是否为 0？
sp 和 sscratch 分别保存哪个 stack？
stvec 是否指向 supervisor_trap_entry？
```

## Step 5：加载 user ELF 的符号

kernel 和 user 是两个独立 ELF。GDB 默认只知道 kernel symbols。

```gdb
add-symbol-file build/solution/user.elf 0x80400000
break user_probe_ecall
break probe_returned
continue
```

在 `user_probe_ecall`：

```gdb
info registers pc a0 a7 sp
x/4i $pc
```

要回答：

```text
a7 是否为 SYS_PROBE？
pc 是否位于已加载 user segment？
```

## Step 6：观察 U→S trap

先设置 trap breakpoint：

```gdb
break supervisor_trap_entry
continue
info registers pc scause sepc sstatus sp sscratch a7
```

注意 breakpoint 停在第一条 trap 指令执行前，因此：

```text
mode 已经是 S
pc 已经来自 stvec
sp 仍然是 user sp
sscratch 仍然保存 kernel sp
```

单步执行 stack 交换：

```gdb
x/i $pc
stepi
info registers sp sscratch
```

要回答：

```text
scause 为什么是 8？
sepc 为什么仍指向 ecall？
csrrw 之后 sp 和 sscratch 如何交换？
```

## Step 7：观察返回 U-mode 的准备

```gdb
break before_probe_sret
continue
info registers sepc sstatus a0 sp sscratch
```

要验证：

```text
sepc = 原 ecall 地址 + 4
a0   = 0x391
sp   = kernel stack（尚未执行最后一次交换）
```

继续：

```gdb
continue
```

程序应停在 `probe_returned`：

```gdb
info registers pc a0 sp
x/4i $pc
```

## Step 8：观察第二次 `ecall` 和退出

```gdb
break user_exit_ecall
break kernel_exit_success
continue
info registers a0 a7
continue
```

预期：

```text
a0 = 0    success exit code
a7 = 2    SYS_EXIT
```

到达 `kernel_exit_success` 证明完整路径成功。

---

# 最小保存区为什么不等于 trap frame？

`trap.S` 只保存：

```text
ra
t0-t2
a0
a7
user sp
sepc
```

它只足以运行本实验受控的 `probe.S`。它不支持：

- trap 发生在任意指令；
- 保存所有通用寄存器；
- handler 任意调用 C 函数后返回；
- nested trap；
- asynchronous interrupt。

思考：

> 如果 timer interrupt 可以发生在用户程序使用 `a1`、`a2` 或 `t3` 的任意时刻，而 handler 又修改了这些寄存器，会发生什么？

这个问题将在 Lab 15 的完整 trap frame 中解决。

---

# Review Questions

1. CSR 和普通寄存器的核心区别是什么？
2. `mret` 的目标 PC 和目标 mode 分别来自哪里？
3. `sret` 的目标 PC 和目标 mode 分别来自哪里？
4. `stvec`、`scause` 和 `sepc` 分别回答什么问题？
5. U-mode `ecall` 时 CPU 是否自动保存全部通用寄存器？
6. 为什么 trap entry 开始时 `sp` 仍然指向 user stack？
7. `csrrw sp, sscratch, sp` 前后两个值如何变化？
8. 为什么 handler 必须让 `sepc += 4`？
9. 为什么 `sepc += 4` 不能作为所有 exception 的通用处理？
10. 为什么 ELF 的 embedded address、load address 和 entry address 不能混为一谈？
11. 为什么本实验的保存区不能处理异步 interrupt？

## 简短答案

1. 普通寄存器保存程序数据；CSR 描述或控制 CPU 状态。
2. `mepc` 和 `mstatus.MPP`。
3. `sepc` 和 `sstatus.SPP`。
4. trap 去哪里、为什么发生、从哪里返回。
5. 不会，保存通用寄存器是 trap entry 的责任。
6. trap 硬件不会自动切换 stack。
7. `sp` 取得旧 `sscratch`，`sscratch` 取得旧 `sp`。
8. 否则 `sret` 会回到同一个 `ecall`，形成重复 trap。
9. 不同 exception 的恢复策略不同，触发指令长度和是否重试也可能不同。
10. 它们分别描述文件来源、segment 目标和第一条用户指令。
11. 异步 interrupt 可发生在任意指令，必须保存所有可能正在使用的状态。

---

# 自动检查

```bash
make MODE=solution check
make MODE=exercise check
```

该命令检查：

- kernel 和 user ELF 能构建；
- user entry 是 `0x80400000`；
- user ELF 含 `PT_LOAD`；
- kernel/user checkpoints 都存在。

安装 QEMU 后，还可以执行端到端检查：

```bash
make MODE=solution run-check
```

该检查实际启动 QEMU，并要求输出包含：

```text
LAB14 PASS
```

构建检查不能代替 GDB 状态解释。实验成功标准是：你能用 CSR 和寄存器证据解释每次控制转移，而不只是看到 `LAB14 PASS`。
