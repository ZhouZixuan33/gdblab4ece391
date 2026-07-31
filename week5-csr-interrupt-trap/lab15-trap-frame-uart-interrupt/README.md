# Lab 15：完整 Trap Frame 与 UART External Interrupt

## 这个实验在做什么？

Lab 14 使用受控的 `ecall` 完成了：

```text
U-mode
  -> S-mode trap handler
  -> U-mode
```

但 Lab 14 只保存了固定测试程序会用到的少量寄存器。真实 hardware interrupt 可以在任意用户指令之间发生，内核无法提前知道哪些寄存器正在使用。

Lab 15 解决两个紧密相关的问题：

1. 怎样保存和恢复完整 trap frame？
2. UART interrupt 怎样经过 PLIC 到达 S-mode handler？

完整路径是：

```text
UART receives one byte
  -> UART raises interrupt source 10
  -> PLIC reports it to hart 0 S-mode
  -> CPU jumps to stvec
  -> trap.S saves a complete trap frame
  -> C dispatcher reads scause
  -> PLIC claim
  -> UART ISR reads the byte
  -> PLIC complete
  -> trap.S restores the trap frame
  -> sret
  -> user program continues with unchanged registers
```

预计时间：1–2 小时。

本实验对应 ECE391-L5 的 hardware interrupt、CSR 和 PLIC，以及 ECE391-L6 的 trap handler、trap frame、interrupt dispatch 和 claim/complete。

---

# 完成实验后，你应该能够

1. 解释 CPU 进入 trap 时自动保存什么；
2. 解释为什么异步 interrupt 必须保存完整通用寄存器状态；
3. 把 `struct trap_frame` 字段与 `trap.S` offset 对应起来；
4. 区分 trap entry、trap dispatcher 和 UART ISR；
5. 根据 `scause` 区分 exception 与 interrupt；
6. 解释 UART、PLIC 和 CPU 三层 interrupt enable；
7. 解释 PLIC claim 和 complete 为什么必须成对出现；
8. 使用 GDB 检查 trap frame、PLIC source ID 和返回后的用户寄存器。

---

# Concept Warm-up

先读完这一部分，再修改 TODO。重点不是背地址，而是能预测每一步是谁完成的。

## 1. 什么是异步 interrupt？

Lab 14 的 `ecall` 是同步 exception：

```text
用户程序主动执行 ecall
  -> trap 一定发生在 ecall 这条指令
```

UART interrupt 是异步事件：

```text
用户什么时候输入字符
  -> CPU 无法提前知道
  -> interrupt 可能发生在任意用户指令之间
```

假设用户程序正在执行：

```asm
add t3, t1, t2
```

UART 可能正好在这条指令之后产生 interrupt。此时 `t1`、`t2`、`t3` 都可能保存用户程序仍然需要的值。

因此内核不能说：

> “C calling convention 允许 handler 修改 caller-saved registers，所以不用保存。”

用户程序没有调用 interrupt handler。Interrupt 是 CPU 强制插入的控制转移。返回以后，用户程序应该像什么都没有发生一样继续执行。

## 2. CPU 进入 trap 时自动保存什么？

当 delegated UART interrupt 到达 S-mode 时，CPU 自动更新：

| 状态 | 作用 |
|---|---|
| `sepc` | 保存被中断程序继续执行的位置 |
| `scause` | 记录这是 interrupt 以及具体 cause |
| `sstatus.SPP` | 记录 trap 前的 privilege mode |
| `sstatus.SPIE/SIE` | 保存并暂时关闭 S-mode interrupt enable |
| `pc` | 改为 `stvec` 指向的 trap entry |

CPU不会自动保存：

```text
ra, gp, tp
t0-t6
s0-s11
a0-a7
sp
```

所以完整的寄存器保存必须由 `trap.S` 完成。

## 3. 什么是 trap frame？

Trap frame 是内核在 trap entry 中建立的一块内存，用来保存被中断程序的 CPU 状态。

本实验在 kernel stack 上分配 272 bytes：

```text
kernel stack high address
┌───────────────────────────┐
│ previous kernel stack     │
├───────────────────────────┤  <- sp + 272
│ scause                    │
│ sstatus                   │
│ sepc                      │
│ original user sp          │
│ t6 ... t0                 │
│ s11 ... s0                │
│ a7 ... a0                 │
│ tp, gp, ra                │
└───────────────────────────┘  <- sp = trap_frame
kernel stack low address
```

`trap_frame.h` 同时定义：

- C 看到的 `struct trap_frame`；
- 汇编使用的 `TF_RA`、`TF_A0`、`TF_SEPC` 等 offset；
- 总大小 `TF_SIZE = 272`。

例如：

```c
frame->a0
```

和：

```asm
ld a0, TF_A0(sp)
```

必须访问同一个位置。

## 4. Trap frame 和普通 C stack frame 有什么区别？

普通 C 函数是程序主动 `call` 的。编译器按照 calling convention 决定 caller 和 callee 各自保存什么。

Trap 不是普通函数调用：

```text
没有 caller 主动 call
可能发生在任意指令
必须能完整恢复被打断的状态
```

因此 trap entry 先用汇编保存完整状态，之后才可以安全调用 C dispatcher。

## 5. 为什么先交换 user stack 和 kernel stack？

刚从 U-mode 进入 trap entry 时：

```text
sp       = user stack
sscratch = kernel stack
```

CPU 不会自动切换 `sp`。`trap.S` 首先执行：

```asm
csrrw sp, sscratch, sp
```

结果：

```text
sp       = kernel stack
sscratch = original user stack
```

随后才能在 kernel stack 上分配 trap frame。原来的 user `sp` 从 `sscratch` 读出并写入 `frame->sp`。

## 6. `scause` 怎样区分 exception 和 interrupt？

RV64 的 `scause`：

```text
bit 63      0 = exception
            1 = interrupt

bits 62:0   cause code
```

本实验的两个例子：

| 事件 | `scause` |
|---|---|
| U-mode `ecall` | `0x0000000000000008` |
| S-mode external interrupt | `0x8000000000000009` |

所以不能只比较低位 cause code。要先检查最高位，再读取 code。

## 7. 什么是 PLIC？

PLIC 是 **Platform-Level Interrupt Controller**，即平台级中断控制器。

CPU 的 external interrupt 入口数量有限，但系统中可能同时存在 UART、磁盘和网络设备等多个 interrupt source。PLIC 位于设备和 CPU 之间：

```text
UART、VirtIO 等设备
        ↓ interrupt request
       PLIC
  记录 pending
  比较 priority
  检查 enable 和 threshold
        ↓
CPU 的 external interrupt
```

QEMU `virt` 将 PLIC 的 MMIO 区域映射到 `0x0c000000`。CPU 对这段地址执行 load/store 时，访问的是 PLIC 硬件寄存器，而不是普通 RAM。PLIC priority、enable、threshold 和 claim/complete register 都位于这段区域中的固定 offset。

PLIC 负责选择并报告“哪个设备需要处理”，但不会替设备完成工作。例如，PLIC 可以报告 UART source 10，真正读取 UART 数据的仍然是 UART ISR。

本实验将 PLIC 访问单独放在两个文件中：

| 文件 | 作用 |
|---|---|
| `plic.h` | 声明内核其他模块可以调用的 PLIC 接口 |
| `plic.c` | 实现 priority、enable、threshold、claim 和 complete 等 MMIO 操作 |

这样 `trap.c` 只需要调用：

```c
plic_enable_uart();
source = plic_claim();
plic_complete(source);
```

不需要在 trap dispatcher 中重复 PLIC 地址和寄存器计算。

学生需要掌握：

- 为什么多个设备需要 interrupt controller；
- 本实验 UART 的 source ID 是 10；
- priority、enable 和 threshold 分别控制什么；
- `claim()` 返回具体 source ID；
- 处理完成后必须用同一个 ID 调用 `complete()`；
- 能读懂并调用 `plic.c/.h` 提供的接口。

学生不需要背诵 PLIC MMIO 地址，也不需要从零实现通用 PLIC driver、多 hart、多 context 或复杂 priority policy。`plic.c` 和 `plic.h` 是框架代码，不是学生 TODO。

### ECE391 要求掌握到什么程度？

Lab 15 的主线已经完成 `plic.c/.h`，学生只在 `trap.c` 中调用这些接口，避免同时调试 trap frame 和底层 PLIC 地址。

但从 ECE391 的学习目标看，学生在已经给出 MMIO 宏、UART source ID 和 S-mode context 的情况下，应该能够写出以下三个最小函数：

```text
plic_enable_uart()
  设置 UART priority
  enable UART source
  设置 threshold

plic_claim()
  读取 claim register
  返回 source ID

plic_complete(source)
  把 claim 返回的同一个 source ID 写回 complete register
```

也就是说，本实验不要求学生修改 `plic.c`，但希望学生读完其中的注释和三段实现后，能够解释每一行，并能在只保留函数框架时自行补全。学生仍然不需要设计通用 PLIC API，也不需要背诵 MMIO 地址。

## 8. UART interrupt 为什么有三层 enable？

Interrupt 必须通过三个关卡：

```text
UART device
  -> PLIC
  -> CPU
```

| 层 | 本实验设置什么？ | 如果没有设置 |
|---|---|---|
| UART | IER receive-data interrupt enable | UART 收到字节但不发 IRQ |
| PLIC | priority、source enable、threshold | PLIC 不向 CPU 报告 source 10 |
| CPU | `sie.SEIE` 和 interrupt 状态 | CPU 不进入 S-mode trap handler |

另外，M-mode boot 已通过 `mideleg` 把 supervisor external interrupt 委托给 S-mode。它属于框架代码，学生不修改。

注意两个名字：

```text
medeleg  Machine Exception Delegation
         Lab 14 用它委托 U-mode ecall

mideleg  Machine Interrupt Delegation
         Lab 15 用它委托 supervisor external interrupt
```

## 9. PLIC 的 claim 和 complete 是什么？

多个设备可能同时请求 interrupt。CPU 进入 external interrupt handler 后，还不知道具体设备。

读取 PLIC claim register：

```text
source = plic_claim()
```

PLIC 返回当前优先级最高的 pending source。本实验 QEMU `virt` UART 的 source ID 是 10。

处理完成后必须写回同一个 ID：

```text
plic_complete(source)
```

它告诉 PLIC：

> source 10 的这次 interrupt 已经处理完毕。

如果忘记 complete，PLIC 仍认为该 interrupt 正在处理，可能造成重复 trap 或后续 interrupt 无法正常报告。

## 10. 谁负责什么？

```text
trap.S
  保存/恢复完整 CPU 状态
  建立 trap frame

trap.c
  查看 scause
  分发 ecall 或 external interrupt

plic.c
  配置 PLIC
  claim / complete

uart.c
  配置 UART interrupt
  从 UART 读取字节
```

不要把所有工作都放进 `trap.S`。汇编负责必须在寄存器安全之前完成的事情；状态保存完成后，使用 C 实现分发更容易理解和维护。

---

# 实验分为两个阶段

## Part A：先验证完整 Trap Frame

用户程序给以下寄存器写入 sentinel：

```text
s1 = 0x151
s2 = 0x252
s3 = 0x353
t3 = 0x454
a1 = 0x515
```

然后执行不会改变返回值的 register-check `ecall`：

```text
save trap frame
  -> C dispatcher
  -> sepc += 4
  -> restore trap frame
  -> sret
```

返回 U-mode 后，用户程序检查 sentinel 是否仍然相同。

Part A 通过后输出：

```text
[S] full trap frame ok
```

## Part B：再开启真实 UART Interrupt

内核启用 UART、PLIC 和 CPU interrupt。用户程序等待串口输入。

在 QEMU terminal 输入一个字符，例如 `Z`：

```text
[S] UART interrupt: source 10
[S] received: Z
[U] register sentinels preserved
[S] user exit: 0
LAB15 PASS
```

用户程序在 external interrupt 返回后再次检查 sentinel。

---

# 学生需要完成什么？

源码包含六个 `LAB15 TODO`：

| TODO | 文件 | 任务 |
|---|---|---|
| 1 | `trap.S` | 把 `s1` 保存到正确 trap-frame slot |
| 2 | `trap.S` | 把 trap frame 地址作为 `a0` 传给 C |
| 3 | `trap.c` | 根据 `scause` 分发 ecall 和 external interrupt |
| 4 | `trap.c` | 启用 UART、PLIC 和 CPU external interrupt |
| 5 | `trap.c` | PLIC claim → UART ISR → complete |
| 6 | `trap.S` | 从正确 slot 恢复 `s1` |

TODO 1 和 TODO 6 故意只留下一个错误 slot，避免把实验变成机械地抄写几十条 `sd/ld`。其余完整保存代码已经展示，学生需要理解整个布局。

---

# 实验文件与调用关系

```text
boot.S
  框架：PMP、medeleg、mideleg、M→S
    ↓ mret

kernel.c: supervisor_entry
  框架：stvec、ELF loader、进入 U-mode
    ↓ sret

user/interrupt_probe.S
  Part A register-check ecall
    ↓ trap

trap.S: supervisor_trap_entry
  TODO 1/2/6：完整 trap frame
    ↓ call

trap.c: supervisor_trap_dispatch
  TODO 3：根据 scause 分发
  TODO 4：启用 interrupt
  TODO 5：external interrupt 路径
    ↓

plic.c                 uart.c
claim/complete         UART receive ISR
```

Part B 发生的真实 UART interrupt 路径是：

```text
在 QEMU terminal 输入字符
    ↓
QEMU NS16550A UART
    收到字符并产生 IRQ
    ↓
PLIC
    将 UART source 10 标记为 pending
    检查 priority、enable、threshold
    ↓
RISC-V CPU
    scause = 0x8000000000000009
    pc = stvec
    ↓
trap.S: supervisor_trap_entry
    切换到 kernel stack
    保存完整 trap frame
    ↓
trap.c: supervisor_trap_dispatch
    根据 scause 判断 external interrupt
    ↓
trap.c: handle_external_interrupt
    plic_claim() → source 10
    uart_receive_interrupt() → 读取字符
    plic_complete(10)
    ↓
trap.S
    恢复完整 trap frame
    sret
    ↓
user/interrupt_probe.S
    继续执行并检查 register sentinels
```

图中的 UART→PLIC→CPU 和 CPU→`stvec` 是硬件控制流，不是普通 C 函数调用。后面的 Guided Mode 会用以下断点逐段验证这条路径：

| Guided Mode 断点 | 正在观察什么 |
|---|---|
| `user_regcheck_ecall` | interrupt 前的用户寄存器基线 |
| `trap_frame_saved` | 完整 trap frame 是否正确 |
| `enable_uart_external_interrupt` | 三层 interrupt enable |
| `supervisor_trap_entry` | CPU 是否收到真实 external interrupt |
| `plic_claim` | PLIC 报告的是哪个设备 |
| `plic_complete` | 是否正确结束这次 interrupt |
| `user_after_uart` | `sret` 后用户寄存器是否保持不变 |

文件清单：

| 文件 | 学生是否修改 | 作用 |
|---|---|---|
| `trap.S` | 是 | trap frame 保存和恢复 |
| `trap.c` | 是 | trap/interrupt dispatcher |
| `trap_frame.h` | 阅读 | C struct 与汇编 offset |
| `plic.c/.h` | 阅读，不修改 | 框架提供的 PLIC MMIO helper；学生在 `trap.c` 中调用接口 |
| `uart.c/.h` | 阅读 | UART output、enable 和 receive |
| `user/interrupt_probe.S` | 阅读 | sentinel 与测试流程 |
| `boot.S` | 否 | M-mode 框架和 delegation |
| `enter_user.S` | 否 | Lab 14 已完成的 S→U |
| `elf_loader.c/.h` | 否 | 已完成的 ELF loader |
| `kernel.c` | 否 | S-mode kernel 框架 |
| `Makefile`、linker scripts | 否 | 构建和地址布局 |

---

# 构建与运行

## 1. 检查工具

```bash
make check-tools
```

Ubuntu/WSL 安装命令：

```bash
sudo apt update
sudo apt install \
  qemu-system-misc \
  gcc-riscv64-unknown-elf \
  binutils-riscv64-unknown-elf \
  gdb-multiarch \
  make
```

## 2. 先运行 solution

```bash
make MODE=solution clean
make MODE=solution
make MODE=solution run
```

看到：

```text
[S] full trap frame ok
```

以后，在同一个 QEMU terminal 输入一个普通字符，例如：

```text
Z
```

应该看到 `LAB15 PASS`。

退出 QEMU：

```text
Ctrl+A，然后按 X
```

## 3. 自动检查

```bash
make MODE=solution check
make MODE=solution run-check
```

`run-check` 会自动向 QEMU 输入字符 `Z`。

## 4. 构建 exercise

```bash
make MODE=exercise clean
make MODE=exercise
make MODE=exercise debug
```

在另一个 terminal：

```bash
make MODE=exercise gdb
```

solution 与 exercise 分别放在：

```text
build/solution/
build/exercise/
```

---

# Guided Mode

本 Guided Mode 默认使用 `MODE=exercise`。开始前先运行一次 `MODE=solution`，确认 QEMU、UART 输入和工具链都能到达 `LAB15 PASS`；随后在 exercise 中观察错误、修复一小组 TODO、重启并重新验证。

## Step 1：在修改前预测

回答：

```text
1. CPU 自动保存了哪些 trap 状态？
2. 为什么不能只保存 caller-saved registers？
3. external interrupt 的 scause 应是多少？
4. UART、PLIC、CPU 三层分别要打开什么？
5. claim 返回 10 后，complete 应该写回什么？
```

未修改的 exercise 通常输出：

```text
[M] boot
[S] kernel
[S] user ELF loaded
[S] unexpected trap; inspect trap frame in GDB
```

不要看到这行就立即修改所有 TODO。下面先用断点证明错误发生在哪里。

## Step 2：建立用户寄存器基线

启动 QEMU 和 GDB 后：

```gdb
add-symbol-file build/exercise/user.elf 0x80400000
break user_regcheck_ecall
break trap_frame_saved
break supervisor_trap_dispatch
break kernel_unexpected_trap
continue
info registers s1 s2 s3 t3 a1 a7
x/i $pc
```

应看到：

```text
s1 = 0x151
s2 = 0x252
s3 = 0x353
t3 = 0x454
a1 = 0x515
a7 = 1
```

这证明用户程序进入 trap 前的状态正确。如果返回后寄存器不同，问题发生在 trap 的保存或恢复路径。

## Step 3：先观察错误的 trap frame，不要立即修复

```gdb
continue
info registers sp sscratch sepc scause
p/x $sp
p/x ((struct trap_frame *)$sp)->s1
p/x ((struct trap_frame *)$sp)->scause
p/x ((struct trap_frame *)$sp)->sepc
```

这里 `sp` 指向 kernel stack 上的 trap frame，`sscratch` 保存 original user `sp`。

未修改的 exercise 应显示类似：

```text
frame->s1     = 0
frame->scause = 8
frame->sepc   = user_regcheck_ecall
```

这说明 `ecall` 正常到达 S-mode，`scause` 和 `sepc` 也保存正确，但 `s1` 没有保存用户原来的 `0x151`。TODO 1 中：

```asm
sd zero, TF_S1(sp)    # 错误：把 0 写进 s1 slot
```

现在只记录结论，先继续观察 TODO 2。

## Step 4：跟踪传给 C dispatcher 的指针

断点仍停在 `trap_frame_saved`。查看并执行下一条指令：

```gdb
x/i $pc
nexti
info registers sp a0
p/d $a0 - $sp
```

未修改时会看到：

```text
a0 = sp + 272
```

但是函数声明是：

```c
void supervisor_trap_dispatch(struct trap_frame *frame);
```

按照 RISC-V calling convention，第一个参数在 `a0`。正确参数应该是 trap-frame base：

```text
a0 = sp
```

继续进入 C dispatcher：

```gdb
continue
p/x frame
p/x $sp
p/x frame->scause
```

因为 TODO 2 传入了错误地址，`frame->scause` 不是刚才看到的 8。继续执行会停在 `kernel_unexpected_trap`，这解释了 QEMU terminal 中的：

```text
[S] unexpected trap; inspect trap frame in GDB
```

## Step 5：只修复 TODO 1、2，然后重启验证

先修复：

```text
TODO 1  将用户 s1 保存到 TF_S1
TODO 2  将 trap-frame base 放入 a0
```

每次修改后的完整调试循环是：

```text
停止旧 GDB 和 QEMU
  -> 重新构建
  -> 启动新的 debug QEMU
  -> 重新连接 GDB
  -> 重新设置断点
```

GDB terminal：

```gdb
disconnect
quit
```

QEMU terminal：

```text
Ctrl+A，然后按 X
```

重新构建并启动：

```bash
make MODE=exercise
make MODE=exercise debug
```

另一个 terminal：

```bash
make MODE=exercise gdb
```

旧断点不会自动进入新的 GDB 会话，需要重新设置。

修复 TODO 1、2 后，不应再因为错误 frame pointer 进入 `kernel_unexpected_trap`。但程序仍会进入 `user_fail`，因为恢复路径中还有 TODO 6。

## Step 6：观察恢复路径，再修复 TODO 6

重新设置：

```gdb
add-symbol-file build/exercise/user.elf 0x80400000
break before_trap_sret
break user_fail
continue
```

在 `before_trap_sret`，trap frame 尚未释放，但通用寄存器已经恢复。比较：

```gdb
p/x ((struct trap_frame *)$sp)->s1
p/x $s1
```

预期发现：

```text
frame 中保存的 s1 = 0x151
即将返回用户的 s1 != 0x151
```

这证明 save 已经正确，错误位于 restore。检查 TODO 6：

```asm
ld s1, TF_S0(sp)      # 错误：从 s0 slot 恢复 s1
```

修复 TODO 6 后，按 Step 5 的循环重新启动。Part A 应通过，用户程序随后执行 enable-interrupt `ecall`，内核输出：

```text
[S] full trap frame ok
```

此时不要修 TODO 3。exercise 已经能处理同步 `ecall`；TODO 3 缺少的是 external-interrupt 分支，留到真实 UART interrupt 到达后观察。

## Step 7：观察并修复 TODO 4 的三层 enable

未修复 TODO 4 时，UART receive interrupt 被打开，但 PLIC 和 CPU 的 enable 仍不完整。设置：

```gdb
break user_wait_for_uart
continue
```

用户到达等待循环后检查：

```gdb
info registers sie sstatus
p/x *(unsigned int *)0x0c000028
p/x *(unsigned int *)0x0c002080
p/x *(unsigned int *)0x0c201000
```

这些地址分别对应：

```text
UART source 10 priority
hart 0 S-mode source enable
hart 0 S-mode threshold
```

UART IER 位于：

```gdb
p/x *(unsigned char *)0x10000001
```

未修复时应发现：

```text
UART IER receive bit 已设置
PLIC UART priority/source enable 尚未正确设置
sie.SEIE 尚未设置
```

这解释了为什么输入字符后 CPU 不进入 trap：三层链路没有全部接通。

修复 TODO 4，按 Step 5 的循环重启，再检查：

```text
UART source 10 priority = 1
PLIC enable register 的 bit 10 = 1
threshold = 0
sie.SEIE = 1
```

## Step 8：触发真实 UART interrupt，再修复 TODO 3

先设置断点：

```gdb
break supervisor_trap_entry
break supervisor_trap_dispatch
break kernel_unexpected_trap
continue
```

然后在 QEMU terminal 输入一个字符。

停下后：

```gdb
info registers pc scause sepc sstatus sie sip
p/x $scause
```

预期：

```text
scause = 0x8000000000000009
```

继续到 `supervisor_trap_dispatch`：

```gdb
continue
p/x frame->scause
```

未修复 TODO 3 时，exercise 只接受 `scause` code 8 的 `ecall`，因此 external interrupt 会进入 `kernel_unexpected_trap`。

修复 TODO 3，使 dispatcher：

```text
先检查 scause interrupt bit
  -> code 9 调用 handle_external_interrupt
  -> exception code 8 处理 ecall
  -> 其他 cause 进入 failure checkpoint
```

然后按 Step 5 的循环重新启动。

## Step 9：观察 claim，证明 TODO 5 缺少 complete

重新启动并设置断点后，执行 `continue`，然后在 QEMU terminal 再输入一个字符：

```gdb
break plic_claim
break plic_complete
continue
```

在 `plic_claim` 返回后检查：

```gdb
finish
next
p/x lab15_last_claim
```

预期 source ID：

```text
10
```

未修复 TODO 5 时，`plic_complete` 断点不会命中，用户程序也会继续停在 `user_wait_for_uart`。占位代码虽然从 UART 读走了字符，但没有发布完成标志，也没有通知 PLIC 处理完成。

修复 TODO 5，确保：

```text
claim source
  -> 确认 source == UART0_IRQ
  -> UART ISR 读取字符
  -> complete 同一个 source
```

重启后，`plic_complete` 断点应该命中。在该函数入口检查第一个参数：

```gdb
info registers a0
```

预期：

```text
a0 = 10
```

## Step 10：验证最终返回后的寄存器

修复 TODO 5 并重启后设置断点。执行 `continue`，再在 QEMU terminal 输入一个字符：

```gdb
break user_after_uart
continue
info registers s1 s2 s3 t3 a1
```

应该仍然是：

```text
s1 = 0x151
s2 = 0x252
s3 = 0x353
t3 = 0x454
a1 = 0x515
```

对 `sret` 这类跨 privilege 指令，不要求使用 `stepi`。部分 QEMU/GDB 版本可能不能正常返回单步提示。使用目标断点：

```gdb
tbreak user_after_uart
continue
```

---

# 常见问题

## Part A 就失败

先检查：

```text
保存 offset 和恢复 offset 是否相同？
a0 是否指向 trap frame base？
sepc 是否跳过了 ecall？
```

不要先调试 PLIC。

## 输入字符后没有进入 trap

按三层顺序检查：

```text
UART IER
  -> PLIC priority / enable / threshold
  -> sie.SEIE
```

同时确认 `stvec` 已经设置。

## 不断重复进入 external interrupt

最常见原因：

```text
读取了 claim
处理了 UART
但没有 complete
```

## `scause` 是 9，为什么判断 interrupt 仍然失败？

因为完整的值是：

```text
0x8000000000000009
```

最高位表示 interrupt。要先分离 interrupt bit 和低位 cause code。

---

# Review Questions

1. 为什么 `ecall` 的最小保存区不能安全处理 UART interrupt？
2. CPU 进入 S-mode trap 时为什么没有自动保存 `a0`？
3. 为什么必须在调用 C dispatcher 之前保存寄存器？
4. `frame->a0` 和 `TF_A0(sp)` 为什么必须对应同一地址？
5. `scause = 0x8000000000000009` 中最高位和低位 9 分别表示什么？
6. UART IER 已启用但 PLIC source enable 没有设置，会发生什么？
7. 为什么 PLIC claim 返回 10？
8. 为什么 complete 必须写回 claim 返回的 source ID？
9. 为什么 trap frame 保存全部寄存器，而 cooperative function call 不一定需要？
10. 为什么本实验不允许 nested interrupt？

## 简短答案

1. UART interrupt 可发生在任意指令，无法预知哪些寄存器正在使用。
2. RISC-V trap 硬件只自动保存必要 CSR 状态，不保存通用寄存器。
3. C 函数可能覆盖通用寄存器。
4. C 和汇编必须对 trap frame 内存布局有相同理解。
5. 最高位表示 interrupt，9 表示 supervisor external interrupt。
6. UART 会请求 interrupt，但 PLIC 不会把 source 10 报告给该 context。
7. QEMU `virt` 的 UART interrupt source ID 是 10。
8. PLIC 需要知道哪一次 in-service interrupt 已处理完成。
9. Trap 是异步强制转移；普通函数调用遵守 calling convention。
10. Nested interrupt 需要额外的重入、stack 和 interrupt-state 设计，不属于本实验范围。

---

# 本实验不包含

- thread 或 context switch；
- scheduler；
- timer-driven preemption；
- nested interrupt；
- ring buffer；
- 通用 device-driver interface；
- 页表或用户地址隔离；
- 从零实现 ELF loader。
