# Lab 14 特权级切换与最小 Trap 往返教学设计

## 1. 定位

Lab 14 是一个面向初学者、预计 1–2 小时完成的混合型实验。学生补全少量关键代码，并使用 QEMU/GDB 证明 RISC-V CPU 完成以下控制流：

```text
M-mode boot
  -> S-mode kernel
  -> load an independent ELF user program
  -> U-mode program
  -> S-mode trap handler
  -> U-mode program
  -> S-mode exit handler
```

本实验是 ECE391 风格 OS 执行路径的最小纵向切片。重点不是实现完整 kernel，而是理解并验证：

- privilege mode 是 CPU 当前状态；
- CSR 如何记录或控制 CPU 状态；
- `mret` 如何完成 M-mode 到 S-mode 的转换；
- `sret` 如何进入或返回 U-mode；
- U-mode `ecall` 如何产生同步 trap；
- kernel 为什么必须修正 `sepc` 才能继续执行用户程序；
- 为什么受控的最小保存区不能替代通用 trap frame。

## 2. 与 ECE391 教学内容的关系

本设计延续 ECE391 L4–L6 的教学顺序：

1. L4 从 MMIO、UART 和 polling 引出 interrupt 以及 handler。
2. L5 建立 M/S/U privilege mode、exception、interrupt、trap、CSR 和 PLIC 的概念。
3. L6 展开 `stvec`、`scause`、`sepc`、`sstatus`、寄存器保存和 `sret`。
4. Lab 14 使用同步 `ecall` 构造可重复、可单步调试的特权级往返。
5. 后续实验再分别引入完整 trap frame、通用 syscall 和异步 interrupt。

ELF loader 与加载用户程序属于后续 OS 执行路径的重要内容，但不是本实验的主要难点。因此 loader 由框架完整提供，ELF 部分只占约 10% 的教学和评分权重。

## 3. 学习目标

完成实验后，学生应能够：

1. 用自己的话解释 CSR 与普通通用寄存器的区别。
2. 对每个重要 CSR 回答“记录什么、谁更新、哪条指令使用”。
3. 根据 `mstatus.MPP` 和 `mepc` 预测 `mret` 的结果。
4. 根据 `sstatus.SPP` 和 `sepc` 预测 `sret` 的结果。
5. 解释 `stvec`、`scause` 和 `sepc` 在 U-mode `ecall` 中的作用。
6. 使用 `sscratch` 和 `csrrw` 解释 user stack 与 kernel stack 的交换。
7. 验证独立 user ELF 的 `PT_LOAD` segment 和 `e_entry`。
8. 使用 GDB 证明 M→S→U→S→U→S 的完整路径。
9. 解释为什么同步 `ecall` 返回前需要执行 `sepc += 4`。
10. 解释最小寄存器保存为什么不适用于任意 trap 或异步 interrupt。

## 4. 范围

### 4.1 包含

- 单 hart、RV64；
- QEMU `virt` 裸机启动；
- M-mode 初始 boot；
- PMP 与 exception delegation 的已完成配置；
- S-mode kernel；
- 已完成的最小 ELF64 RISC-V loader；
- 独立构建并嵌入 kernel image 的 user ELF；
- `satp = 0` 的无分页执行环境；
- 两个受控的 `ecall`；
- 最小寄存器保存和 user/kernel stack 交换；
- GDB checkpoints、状态预测和端到端检查。

### 4.2 不包含

- 页表、虚拟内存和用户地址空间隔离；
- 从零实现 ELF parser；
- section、dynamic linking、relocation；
- 完整 trap frame；
- 嵌套 trap；
- 通用 syscall table；
- 用户指针检查；
- timer interrupt；
- UART、PLIC 或其他外部 interrupt；
- 多 hart。

## 5. 执行架构

### 5.1 M-mode boot

QEMU reset 后，CPU 从 M-mode 开始执行。框架完成 M-mode stack、PMP 和 delegation 的基础配置。学生补全：

1. 将 `mstatus.MPP` 设置为 S；
2. 将 `supervisor_entry` 写入 `mepc`；
3. 执行 `mret`。

`before_mret` 是稳定的 GDB checkpoint。学生必须在此预测并验证：

```text
next pc   = mepc
next mode = mstatus.MPP
```

### 5.2 S-mode kernel 与 ELF load

S-mode kernel：

1. 将 `supervisor_trap_entry` 写入 `stvec`；
2. 调用已提供的最小 ELF loader；
3. 检查 ELF64、RISC-V machine type 和 program header；
4. 复制并清零 `PT_LOAD` segment；
5. 返回 `e_entry` 和已加载 segment 的范围。

user ELF 是独立链接产物，通过 `.incbin` 或等价构建步骤嵌入 kernel image。loader 在运行时解析真实 ELF 字节，不使用文件系统。

学生不补全 loader，只回答：

```text
e_entry 是多少？
PT_LOAD 被加载到哪里？
为什么 sepc 使用 e_entry，而不是 embedded ELF 的源地址？
```

### 5.3 S-mode 进入 U-mode

学生补全：

1. 将 ELF entry 写入 `sepc`；
2. 清除 `sstatus.SPP`，选择 U-mode；
3. 设置 user stack；
4. 将 kernel stack pointer 写入 `sscratch`；
5. 执行 `sret`。

`before_user_sret` checkpoint 必须允许学生检查：

```text
sstatus.SPP
sepc
sp
sscratch
```

### 5.4 第一次 `ecall`：probe

用户程序使用最小调用约定：

```text
a7 = 1        SYS_PROBE
ecall
```

trap entry 使用：

```asm
csrrw sp, sscratch, sp
```

交换 user stack 与 kernel stack，并保存该 handler 会覆盖的最小寄存器集合：

```text
ra
t0-t2
a0
a7
user sp
sepc
```

handler：

1. 验证 `scause` 是 environment call from U-mode；
2. 读取 `a7`；
3. 对 `SYS_PROBE` 执行 `sepc += 4`；
4. 设置 `a0 = 0x391`；
5. 恢复寄存器，但保留新的 `a0`；
6. 换回 user stack；
7. 执行 `sret`。

### 5.5 第二次 `ecall`：exit

用户程序确认 `a0 == 0x391` 后执行：

```text
a7 = 2        SYS_EXIT
a0 = 0        success
ecall
```

kernel 识别 `SYS_EXIT`，记录退出码并进入稳定停机点：

```text
kernel_exit_success
kernel_exit_failure
```

本实验不要求从 exit 返回 U-mode。

## 6. Concept Warm-up

Concept Warm-up 是本实验的必要组成部分，预计 20–25 分钟。它必须先建立模型，再介绍代码和命令，不能以 CSR 名称清单代替解释。

### 6.1 什么是 CSR

首次出现 CSR 时使用以下定义：

> CSR 是 Control and Status Register，即控制与状态寄存器。普通寄存器保存程序计算的数据；CSR 描述或控制 CPU 自身的运行状态。

学生不需要背 CSR 地址。每遇到一个 CSR，都使用三个问题组织理解：

```text
1. 它记录或控制什么？
2. 谁更新它：CPU 硬件还是 kernel？
3. 哪个事件或返回指令使用它？
```

### 6.2 本实验的重要 CSR

| CSR | 英文全称与缩写拆解 | 初学者需要掌握的含义 | 谁更新 | 谁使用 |
|---|---|---|---|---|
| `mstatus.MPP` | **M**achine **Status** Register / **M**achine **P**revious **P**rivilege mode | `mret` 返回后的 mode | kernel | `mret` |
| `mepc` | **M**achine **E**xception **P**rogram **C**ounter | `mret` 的目标 PC | kernel 或 M-mode trap 硬件 | `mret` |
| `medeleg` | **M**achine **E**xception **Deleg**ation Register | 哪些 exception 交给 S-mode | 框架中的 M-mode boot | trap 硬件 |
| `sstatus.SPP` | **S**upervisor **Status** Register / **S**upervisor **P**revious **P**rivilege mode | trap 前的 mode，也是 `sret` 选择的返回 mode | trap 硬件；kernel 可修改 | `sret` |
| `sepc` | **S**upervisor **E**xception **P**rogram **C**ounter | trap 相关 PC，也是 `sret` 的目标 PC | trap 硬件；handler 可修改 | `sret` |
| `stvec` | **S**upervisor **T**rap **Vec**tor Base Address Register | S-mode trap 入口地址 | kernel | trap 硬件 |
| `scause` | **S**upervisor **Cause** Register | trap 原因 | trap 硬件 | handler |
| `sscratch` | **S**upervisor **Scratch** Register | S-mode trap 的临时状态，本实验保存 kernel/user stack | kernel 与 trap entry | trap entry |
| `satp` | **S**upervisor **A**ddress **T**ranslation and **P**rotection Register | 地址转换配置 | kernel | 地址转换硬件 |

表后提供统一助记规则：

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

`satp` 在本实验设为 0。学生只需知道当前不启用页表，不展开其字段。

### 6.3 CSR 访问

README 只介绍实验实际使用的写法：

```asm
csrr t0, sepc
csrw sepc, t0
csrs sstatus, t0
csrc sstatus, t0
csrrw sp, sscratch, sp
```

每条写法必须配类 C 语义和是否有副作用。特别说明 `csrrw` 同时读取旧值并写入新值。

### 6.4 特权级是 CPU 状态

必须明确：

```text
privilege mode 是 CPU 当前状态，
不是某段源代码永久具有的属性。
```

同一地址上的指令是否允许执行，取决于 CPU 执行时的 mode。

### 6.5 普通跳转与特权返回

| 操作 | PC 来源 | 是否改变 mode |
|---|---|---|
| `j/call/ret` | immediate 或通用寄存器 | 否 |
| `mret` | `mepc` | 是，由 `MPP` 决定 |
| `sret` | `sepc` | 是，由 `SPP` 决定 |
| trap | `stvec` | U→S |

配套预测题必须使用具体地址和值，例如：

```text
mepc = 0x80200000
mstatus.MPP = S
mret 后 pc 和 mode 分别是什么？
```

### 6.6 Trap 时硬件和软件的边界

U-mode `ecall` 被委托给 S-mode 时，硬件自动：

```text
sepc   <- ecall address
scause <- environment call from U-mode
SPP    <- U
pc     <- stvec
mode   <- S
```

硬件不会：

```text
保存全部通用寄存器
切换 kernel stack
判断 syscall number
自动跳过 ecall
设置 syscall 返回值
```

handler 必须负责后半部分。README 必须把这两组动作并列展示。

### 6.7 两个 stack 与 `sscratch`

必须用具体数值练习 `csrrw`：

```text
before:
sp       = 0x80410000
sscratch = 0x80208000

csrrw sp, sscratch, sp

after:
sp       = 0x80208000
sscratch = 0x80410000
```

学生随后在 GDB 中验证真实值。

### 6.8 为什么 `sepc += 4`

必须展示错误循环：

```text
ecall at 0x80400004
  -> sepc = 0x80400004
  -> sret without update
  -> execute ecall again
```

本实验的 `ecall` 是固定 32-bit 指令，因此前移 4 bytes。README 必须警告：不能将“所有 exception 都执行 `sepc += 4`”推广为通用规则。

### 6.9 ELF 最小模型

只介绍：

```text
ELF bytes
  -> program headers
  -> copy/zero PT_LOAD
  -> e_entry becomes initial user pc
```

必须区分：

```text
embedded_elf_start
p_vaddr / p_paddr
e_entry
```

不介绍 section header、dynamic linking 或 relocation。

### 6.10 Warm-up 预测表

学生运行 QEMU 前填写：

| Checkpoint | 当前 mode | 下一 PC 来源 | 下一 mode 的依据 |
|---|---|---|---|
| `before_mret` | M | `mepc` | `MPP` |
| `before_user_sret` | S | `sepc` | `SPP` |
| user `ecall` 后 | S | `stvec` | trap 硬件规则 |
| `before_probe_sret` | S | `sepc` | `SPP` |

实验中将预测与 GDB 结果逐项对照。

### 6.11 贯穿实验的具体例子

Concept Warm-up 不能只给抽象定义。README 必须使用同一组具体地址和值，完整演示状态转换。例子中的地址应与最终 linker script 保持一致；如果实现阶段调整内存布局，README、GDB 输出和下列示例必须同步修改。

#### 例 1：从 M-mode 进入 S-mode

假设：

```text
supervisor_entry = 0x80200000
CPU current mode = M
```

kernel 先设置返回地址：

```asm
la   t0, supervisor_entry
csrw mepc, t0
```

再将 `mstatus.MPP` 设置为 S。README 先给字段语义：

```text
MPP 位于 mstatus[12:11]
MPP = 01 表示 S-mode
```

然后展示一种不破坏其他 `mstatus` bits 的写法：

```asm
li   t0, (3 << 11)
csrc mstatus, t0       # 先清除原 MPP
li   t0, (1 << 11)
csrs mstatus, t0       # 再设置 MPP = S
```

执行 `mret` 前，学生填写：

```text
current pc   = before_mret 附近
current mode = M
mepc         = 0x80200000
MPP          = S
```

执行：

```asm
mret
```

结果：

```text
pc           = 0x80200000
current mode = S
```

README 必须说明 `mret` 不是通过 `t0` 跳转；`t0` 只用于把地址写入 `mepc`，真正被 `mret` 读取的是 CSR `mepc`。

#### 例 2：从 S-mode 进入 user ELF

假设 loader 返回：

```text
embedded_elf_start = 0x80210000
loaded PT_LOAD     = 0x80400000 .. 0x80400fff
ELF e_entry        = 0x80400000
user stack top     = 0x80410000
kernel stack top   = 0x80208000
```

进入用户态前使用合法的 CSR 寄存器形式：

```asm
li   t0, 0x80400000
csrw sepc, t0
li   t0, SSTATUS_SPP
csrc sstatus, t0            # SPP = 0，选择 U-mode
li   t0, 0x80208000
csrw sscratch, t0
li   sp, 0x80410000
sret
```

执行 `sret` 后：

```text
pc           = 0x80400000
current mode = U
sp           = 0x80410000
sscratch     = 0x80208000
```

README 必须并列比较：

```text
0x80210000 是 ELF 文件字节在 kernel image 中的位置；
0x80400000 是 segment 加载后的地址，同时是本例的 e_entry；
sepc 必须使用 e_entry，而不是 embedded_elf_start。
```

#### 例 3：U-mode 执行第一次 `ecall`

假设用户代码为：

```asm
li   a7, 1              # SYS_PROBE
ecall                   # address = 0x80400004
probe_returned:
li   t0, 0x391
bne  a0, t0, user_fail
```

执行 `ecall` 前：

```text
mode     = U
pc       = 0x80400004
a7       = 1
sp       = 0x80410000
sscratch = 0x80208000
stvec    = supervisor_trap_entry
```

trap 被委托给 S-mode 后，硬件产生：

```text
mode        = S
pc          = stvec
sepc        = 0x80400004
scause      = 8
sstatus.SPP = 0
sp          = 0x80410000       # 硬件没有自动换栈
```

README 必须要求学生在 GDB 中核对每一项，并解释：

- `scause = 8` 表示 environment call from U-mode；
- `SPP = 0` 说明 trap 前来自 U-mode；
- `sepc` 指向 `ecall`，而不是它后面的指令；
- `sp` 仍是 user stack，证明硬件没有自动切换 kernel stack。

#### 例 4：交换 stack 并返回用户程序

trap entry 执行：

```asm
csrrw sp, sscratch, sp
```

交换后：

```text
sp       = 0x80208000          # kernel stack
sscratch = 0x80410000          # 暂存 user stack
```

handler 识别 `a7 = 1` 后：

```text
old sepc = 0x80400004
new sepc = 0x80400008
new a0   = 0x391
```

恢复最小现场并再次执行 stack 交换后：

```text
sp       = 0x80410000
sscratch = 0x80208000
```

执行 `sret`：

```text
pc           = 0x80400008
current mode = U
a0           = 0x391
```

用户程序因此从 `probe_returned` 继续，而不是再次执行 `ecall`。

#### 例 5：不更新 `sepc` 的失败路径

README 必须用相同地址展示反例：

```text
1. U-mode 在 0x80400004 执行 ecall
2. hardware 写入 sepc = 0x80400004
3. handler 没有执行 sepc += 4
4. sret 令 pc = 0x80400004
5. U-mode 再次执行同一个 ecall
6. 重复步骤 2–5
```

GDB 练习要求学生故意临时观察该故障场景，连续两次停在 `supervisor_trap_entry`，记录两次相同的 `sepc`，然后恢复正确实现。

这些例子必须采用“执行前状态 → 指令或事件 → 执行后状态”的固定结构，帮助学生把 CSR 名称、特权级和控制流连接起来。

## 7. 学生 TODO

设置五个短 TODO，每个只对应一个概念：

1. `boot.S`：设置 `mstatus.MPP = S`；
2. `boot.S`：设置 `mepc = supervisor_entry`；
3. `kernel.c` 或对应 CSR helper：设置 `stvec`；
4. `enter_user.S`：设置 `sepc`、清除 `SPP`、准备 stack；
5. `trap.S`：识别 user `ecall`、执行 `sepc += 4`、返回 `0x391`。

PMP、delegation、ELF parser、segment copy、linker scripts 和用户程序均提供完成版本。

实现提供两个隔离的构建模式：

```text
MODE=solution   五个教学点使用完整参考实现
MODE=exercise   五个教学点使用可构建但行为故意错误的占位
```

构建产物分别写入：

```text
build/solution/
build/exercise/
```

学生先运行 `solution` 建立正常输出基线，再切换到 `exercise`，按照 M→S、S→U、U→S 的顺序逐项修复。两个模式必须保留相同 checkpoint symbols，使 README 中的 GDB 工作流可以直接复用。

## 8. GDB Checkpoints

| Checkpoint | 要证明的事实 | 主要观察 |
|---|---|---|
| `before_mret` | 下一次返回进入 S-mode | `mstatus`、`mepc` |
| `supervisor_entry` | M→S 成功 | `pc` 与可访问的 S-mode CSR |
| `elf_load_done` | user ELF 已加载 | entry、segment range、目标反汇编 |
| `before_user_sret` | 下一次返回进入 U-mode | `sstatus`、`sepc`、`sp`、`sscratch` |
| `user_probe_ecall` | 用户程序到达第一次调用 | `pc`、`a7` |
| `supervisor_trap_entry` | U-mode `ecall` 进入 S-mode | `scause`、`sepc`、`sstatus` |
| `before_probe_sret` | handler 已准备正确返回 | `sepc`、`a0`、`sp`、`sscratch` |
| `probe_returned` | 用户程序收到返回值 | `a0`、用户 `pc` |
| `user_exit_ecall` | 用户发出退出请求 | `a7`、`a0` |
| `kernel_exit_success` | 完整路径成功 | 最终状态 |

每个 checkpoint 的 README 步骤必须说明该命令回答什么问题，不能只给命令列表。

## 9. 文件结构

```text
lab14-privilege-roundtrip/
├── README.md
├── Makefile
├── kernel.ld
├── user.ld
├── boot.S
├── enter_user.S
├── trap.S
├── kernel.c
├── elf_loader.c
├── elf_loader.h
├── user/
│   └── probe.S
└── scripts/
    └── check-lab14.sh
```

文件职责：

- `boot.S`：M-mode boot、PMP、delegation 和 M→S；
- `kernel.c`：S-mode 初始化、ELF load 和 user entry 准备；
- `enter_user.S`：S→U；
- `trap.S`：最小 trap 保存与两个受控 `ecall`；
- `elf_loader.*`：完整提供的最小 loader；
- `user/probe.S`：独立 user ELF；
- `check-lab14.sh`：静态和端到端检查。

## 10. 故障反馈

| 错误 | 预期症状 | 首要证据 |
|---|---|---|
| `MPP` 错误 | `mret` 后未按预期进入 S-mode | `mstatus`、`mepc` |
| `mepc` 错误 | 未到达 `supervisor_entry` | `pc`、symbol address |
| `stvec` 错误 | `ecall` 后进入错误地址 | `stvec`、QEMU trap log |
| `SPP` 未清零 | `sret` 未进入预期 U-mode | `sstatus`、trap 行为 |
| `sepc` 未加 4 | 重复执行同一 `ecall` | `sepc` 不变 |
| 未交换 stack | trap 保存区损坏或访问异常 | `sp`、`sscratch`、内存 |
| 恢复旧 `a0` | user 看不到 `0x391` | 返回路径中的 `a0` |

所有关键阶段均提供稳定 symbol，避免错误只表现为无解释的黑屏。

## 11. 评分

总分 100：

| 项目 | 分值 |
|---|---:|
| M→S 配置与解释 | 15 |
| ELF load 结果解释 | 10 |
| S→U 配置与解释 | 20 |
| stack 交换和最小保存 | 15 |
| `scause` 判断与 `sepc += 4` | 15 |
| `sret` 返回与 `a0` 结果 | 10 |
| GDB 状态表 | 10 |
| 最小 handler 局限解释 | 5 |

自动测试覆盖构建、checkpoint symbol 和端到端结果，但概念解释必须人工检查。

## 12. 验证

实现后必须验证：

1. kernel 和 user ELF 独立构建；
2. `readelf -h/-l user.elf` 显示 ELF64 RISC-V 和有效 `PT_LOAD`；
3. kernel symbol table 包含全部 checkpoints；
4. QEMU 在超时前到达 `kernel_exit_success`；
5. GDB 能在每个 checkpoint 读取设计中声明的 CSR 和寄存器；
6. 第一次 `ecall` 返回 `a0 = 0x391`；
7. 删除 `sepc += 4` 时能稳定复现重复 trap；
8. README 的每个 CSR 定义与真实代码用法一致；
9. README 没有暗示 CPU 会自动保存通用寄存器或自动切换 stack；
10. README 没有将本实验的最小保存区称为完整 trap frame。

预期最终 kernel 输出：

```text
[M] boot
[S] kernel
[S] user ELF loaded
[U] probe returned
[S] user exit: 0
LAB14 PASS
```

在 `satp = 0` 且 PMP 放行的教学环境中，输出标签用于描述逻辑阶段，不代表已经建立安全的用户/内核地址隔离。

## 13. 时间安排

```text
Concept Warm-up             25 分钟
M→S TODO 与验证             15 分钟
观察 ELF loader             10 分钟
S→U TODO 与验证             20 分钟
ecall 往返 TODO 与验证       25 分钟
状态表和复习                 10 分钟
总计                        105 分钟
```

## 14. 后续实验

```text
Lab 14  受控 ecall 的最小特权级往返
Lab 15  完整 trap frame
Lab 16  通用 syscall dispatch
Lab 17  timer interrupt
Lab 18  UART + PLIC external interrupt
```

Lab 15 用完整寄存器布局替换最小保存区，并允许 handler 调用 C。Lab 16 加入 syscall table、参数和错误返回。Lab 17 证明异步 trap 可以发生在任意用户指令上。Lab 18 完成 device、PLIC、CPU 三层 interrupt enable，与 ECE391 L4–L6 的设备中断路径完整对接。
