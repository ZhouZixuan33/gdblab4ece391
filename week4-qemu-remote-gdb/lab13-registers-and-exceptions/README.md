# Lab 13：从寄存器和当前指令定位 RISC-V 低层故障

## 这个实验在做什么？

Lab 12 解决的问题是：

> QEMU 如何进入我们自己编译的裸机程序？

Lab 13 接着研究：

> CPU 已经进入程序以后，如果某条指令使用了错误的地址、跳到了意外的位置，或者根本无法被译码，我们怎样找到最初的原因？

本实验会故意制造三类问题：

```text
bad-pointer          用可疑地址读取内存
bad-jump             通过寄存器跳到意外位置
illegal-instruction  执行无法正常译码的位模式
```

重点不是“让程序崩溃”，也不是记住一组 GDB 命令。重点是建立一条调试推理链：

```text
pc 指向哪条指令？
    -> 这条指令读取哪些寄存器？
    -> 这些寄存器中的值是否合理？
    -> 这些证据能否解释当前现象？
```

完成实验后，你应该能够从当前指令出发，判断接下来该检查哪个寄存器，而不是看到异常后盲目猜测。

## 你将练习什么？

本实验主要使用：

```gdb
info registers
info registers pc sp ra a0
x/i $pc
x/10i $pc
p/x $a0
x/32gx $sp
disassemble
```

其中最重要的组合是：

```gdb
x/i $pc
info registers
```

第一条告诉你 CPU 准备执行什么，第二条告诉你这条指令可以使用哪些输入值。

---

## Concept Warm-up 1：CPU 怎样执行一条指令？

暂时把 CPU 想成不断重复三个动作的机器：

```text
1. 取指：根据 pc 从内存读取一条指令
2. 译码：判断这是什么指令、使用哪些寄存器
3. 执行：计算结果、访问内存，或改变下一条指令的位置
```

`pc`（program counter）保存指令地址。在 GDB 停止程序时：

```gdb
x/i $pc
```

会把 `$pc` 所在位置解释成一条机器指令。它回答的是：

> CPU 当前停在哪条指令上？


## Concept Warm-up 2：怎样读本实验里的 RISC-V 汇编？

### 本实验指令速查表

这张表覆盖 `start.S` 中需要读懂的全部 CPU 指令和常用 pseudoinstruction。第一次阅读时不必全部背下来；遇到不熟悉的指令，先回到这里确认它的输入、输出以及是否访问内存。

表格中的 `Memory[address]` 是为了帮助理解而写的伪代码，表示“地址对应的内存内容”，不是实际的 C 语法。

| 写法 | 英文全称/拆解 | 通俗含义 | 类 C 伪代码 | 是否访问内存 |
|---|---|---|---|---|
| `li a0, 42` | **l**oad **i**mmediate（pseudoinstruction） | 把常数 `42` 放入 `a0` | `a0 = 42` | 否 |
| `la a0, msg` | **l**oad **a**ddress（pseudoinstruction） | 把 label `msg` 的地址放入 `a0` | `a0 = &msg` | 否；这里只取得地址 |
| `mv a1, a0` | **m**o**v**e（pseudoinstruction） | 把 `a0` 的值复制到 `a1` | `a1 = a0` | 否 |
| `ld a1, 0(a0)` | **l**oad **d**oubleword | 从地址 `a0 + 0` 读取 8 bytes，写入 `a1` | `a1 = Memory64[a0 + 0]` | 是：读取 8 bytes |
| `lbu t1, 0(t0)` | **l**oad **b**yte **u**nsigned | 从地址 `t0 + 0` 读取 1 byte，zero-extend 后写入 `t1` | `t1 = ZeroExtend(Memory8[t0])` | 是：读取 1 byte |
| `sb t1, 0(t2)` | **s**tore **b**yte | 把 `t1` 的低 8 bits 写到地址 `t2 + 0` | `Memory8[t2] = t1 & 0xff` | 是：写入 1 byte |
| `addi t0, t0, 1` | **add i**mmediate | `t0` 加 1，结果仍放在 `t0` | `t0 = t0 + 1` | 否 |
| `andi t3, t3, 0x20` | bitwise **AND i**mmediate | `t3` 与 `0x20` 做 bitwise AND | `t3 = t3 & 0x20` | 否 |
| `beqz t1, label` | **b**ranch if **eq**ual to **z**ero（pseudoinstruction） | 如果 `t1 == 0`，跳到 `label` | `if (t1 == 0) goto label` | 否 |
| `j label` | **j**ump（pseudoinstruction） | 无条件跳到 `label` | `goto label` | 否 |
| `jr t0` | **j**ump **r**egister（pseudoinstruction） | 跳到 `t0` 保存的地址 | `pc = t0` | 否；但会改变控制流 |
| `call function` | **call** procedure/function（pseudoinstruction） | 保存返回地址，再跳到函数 | `ra = return_address; goto function` | 指令本身不读写数据内存；被调函数可能会 |
| `ret` | **ret**urn（pseudoinstruction） | 跳回 `ra` 保存的返回地址 | `pc = ra` | 否 |
| `wfi` | **w**ait **f**or **i**nterrupt | 等待 interrupt | `wait_for_interrupt()` | 否 |

标有 pseudoinstruction 的写法是 assembler 提供的方便形式。它们可能被展开成一条或多条基础机器指令，因此在 GDB 的 disassembly 中不一定仍以相同名字出现。

最容易混淆的是 `mv` 和 `ld`：

```text
mv a1, a0      -> a1 = a0             -> 复制寄存器中的数值
ld a1, 0(a0)   -> a1 = Memory64[a0]   -> 把 a0 当作地址，再读取内存
```

看到括号形式 `offset(register)` 时，应先计算有效地址：

```text
effective address = register 中的值 + offset
```

### 2.1 Label 是地址的名字

汇编中的：

```asm
bad_pointer_fault_site:
    ld a1, 0(a0)
```

`bad_pointer_fault_site` 是下一条指令地址的名字。它本身不会执行任何操作。

GDB 可以利用这个名字设置 breakpoint：

```gdb
break bad_pointer_fault_site
```

程序停下时，`pc` 会指向该 label 后面的 `ld` 指令。

### 2.2 先看目标，再看输入

许多 RISC-V 指令采用这样的阅读顺序：

```text
操作  目标, 输入
操作  目标, 输入1, 输入2
```

例如：

```asm
li a0, 0xdeadbeef   # 把立即数 0xdeadbeef 放进 a0
mv t0, a0           # 把 a0 的值复制到 t0
addi t0, t0, 1      # t0 = t0 + 1
andi t3, t3, 0x20   # t3 = t3 & 0x20
```

`li`、`la`、`mv`、`call`、`ret` 等常写成一条汇编语句，但可能由 assembler 展开成一条或多条真实机器指令。它们通常称为 pseudoinstruction（伪指令）。在 GDB 反汇编中看到展开后的形式是正常的。

### 2.3 本实验需要认识的寄存器

你已经在 Lab 12 见过：

```text
pc  当前指令地址
sp  stack pointer，栈顶地址
ra  return address，函数返回位置
```

本实验还会频繁使用：

```text
a0, a1      函数参数/返回值寄存器
t0 - t3     临时寄存器
```

本实验中的 `uart_puts` 接收一个参数：字符串地址放在 `a0`。

`a*` 和 `t*` 都属于 caller-saved registers。最低限度的理解是：

> 调用另一个函数以后，调用者不能假定这些寄存器仍保留调用前的值。

因此 `scenario_bad_pointer` 是在打印完成后，才把 `0xdeadbeef` 放入 `a0`。

### 2.4 地址和常数

比较：

```asm
la a0, msg_start
li a0, 0xdeadbeef
```

- `la`（load address）把 label 对应的地址放入寄存器；
- `li`（load immediate）把一个常数放入寄存器。

`la a0, msg_start` 的意思不是读取字符串内容，而是让 `a0` 指向字符串。

### 2.5 Load 和 Store

本实验中的内存访问包括：

```asm
ld  a1, 0(a0)
lbu t1, 0(t0)
sb  t1, 0(t2)
```

括号表示“以寄存器中的值为基址”：

```text
有效地址 = 基址寄存器 + offset
```

所以：

```asm
ld a1, 0(a0)
```

表示：

```text
从地址 a0 + 0 读取 8 bytes
把读取结果写入 a1
```

如果 `a0 == 0xdeadbeef`，CPU 会尝试从 `0xdeadbeef` 附近读取数据。`a1` 是结果寄存器，`a0` 才是判断地址是否合理的关键。

用一个具体例子比较 `mv` 和 `ld`。假设寄存器和内存当前是：

```text
a0 寄存器                         内存
┌────────────┐                    地址                 内容（8 bytes）
│ 0x80001000 │ ────────────────>  0x80001000          0x123456789abcdef0
└────────────┘
```

执行复制：

```asm
mv a1, a0
```

结果是：

```text
a1 = 0x80001000
```

因为 `mv` 只复制寄存器中的数值，并没有顺着这个地址读取内存。

如果改为：

```asm
ld a1, 0(a0)
```

结果则是：

```text
effective address = 0x80001000 + 0
a1 = Memory64[0x80001000]
a1 = 0x123456789abcdef0
```

可以把 `a0` 想成一张写着仓库位置的纸条：

- `mv a1, a0` 是把纸条上的地址抄到另一张纸上；
- `ld a1, 0(a0)` 是按照纸条找到仓库，再取出那里存放的 8-byte 内容。

另外：

- `lbu`（load byte unsigned）读取 1 byte，并进行 zero extension；
- `sb`（store byte）把寄存器的低 8 bits 写到内存或 MMIO 地址；
- `ld`（load doubleword）在 RV64 中读取 8 bytes。

### 2.6 跳转、调用和返回

```asm
j label
call function
ret
jr t0
```

- `j label`：无条件跳到 label；
- `call function`：把返回地址放入 `ra`，再跳到 function；
- `ret`：跳回 `ra` 保存的返回地址；
- `jr t0`：跳到 `t0` 中保存的地址。

前三种写法直接给出了 label 或遵守函数调用约定。`jr t0` 是 indirect jump（间接跳转），目标要到寄存器中寻找：

```text
当前指令是 jr t0
    -> 下一条指令地址由 t0 决定
    -> 所以应检查 pc 和 t0
```

### 2.7 条件分支和数字局部 Label

UART 循环中有：

```asm
1:
    ...
    beqz t1, 3f
2:
    ...
    beqz t3, 2b
    ...
    j 1b
3:
    ret
```

GNU assembler 允许重复使用数字作为 local label：

```text
2b  跳到当前位置之前最近的 2:（backward）
3f  跳到当前位置之后最近的 3:（forward）
```

`beqz t1, 3f` 表示 `t1 == 0` 时跳到后面的 `3:`。字符串以 `\0` 结尾，所以这是打印结束条件。

### 2.8 `.word` 为什么能制造非法指令？

正常汇编指令会由 assembler 编码成机器码。例如，你写 `ret`，assembler 会生成 CPU 能识别的位模式。

这行则直接把 32-bit 原始值放入代码区：

```asm
.word 0xffffffff
```

`.word` 是 assembler directive，不是 CPU 指令。CPU 执行到这四个 bytes 时，会尝试把 `0xffffffff` 当作一条 RISC-V 指令译码。本实验选择的位模式不是当前 ISA 配置下的合法指令，因此会产生 illegal-instruction exception。

---

## 三个故障场景有什么不同？

先比较它们，再开始调试：

| Scenario | 被破坏的对象 | 关键位置 | 关键寄存器 | 你要证明什么 |
|---|---|---|---|---|
| `bad-pointer` | 数据访问地址 | `ld a1, 0(a0)` | `a0` | load 会使用 `0xdeadbeef` 作为地址 |
| `bad-jump` | 控制流目标 | `jr t0` | `t0` | 下一条指令地址来自 `t0` |
| `illegal-instruction` | 指令译码 | `.word 0xffffffff` | `pc` | CPU 将在这里遇到非法位模式 |

注意：`bad-jump` 并不一定产生 CPU exception。本实验故意让它跳到一个确实存在、但从程序意图来看出乎预期的 label。它展示的是 control-flow failure：

```text
CPU 完全可以正确执行一条跳转指令，
但程序放进目标寄存器的地址可能不是开发者原本想去的位置。
```

## 为什么最好在故障指令执行前停下？

本实验与 Lab 12 一样运行在 M-mode，但没有安装完整的 trap handler。

发生 exception 时，CPU 会记录 trap 原因和相关地址，并转向 `mtvec` 指定的 trap 入口。常见 CSR 包括：

```text
mcause  trap 原因
mepc    发生 trap 时相关的指令地址
mtval   与 trap 有关的附加值
mtvec   trap handler 的入口配置
```

但是本实验没有建立一个可供你依赖的 handler。执行故障指令后，目标可能进入难以解释的状态，原来的线索反而不如执行前直接。

因此主要策略是：

```text
在可疑指令执行前设置 breakpoint
    -> 检查指令
    -> 检查这条指令使用的寄存器
    -> 在不破坏现场的情况下证明根因
```

`mcause`、`mepc`、`mtval` 和 QEMU log 是补充证据，不是本实验唯一的成功标准。

---

## 可用的 Scenario

```bash
make SCENARIO=good
make SCENARIO=bad-pointer
make SCENARIO=bad-jump
make SCENARIO=illegal-instruction
```

- `good`：确认 QEMU、交叉工具链和 UART 输出正常；
- `bad-pointer`：在 load 前令 `a0 = 0xdeadbeef`；
- `bad-jump`：通过 `t0` 跳到意外但有效的位置；
- `illegal-instruction`：执行故意放入代码区的非法位模式。

不同 scenario 使用不同构建目录，例如：

```text
build/good/kernel.elf
build/bad-pointer/kernel.elf
build/bad-jump/kernel.elf
build/illegal-instruction/kernel.elf
```

这能避免不同 scenario 的 ELF 和 object file 相互覆盖。

---

## Guided Mode

### Step 1：检查工具并运行正常场景

```bash
make check-tools
make SCENARIO=good run
```

预期串口输出包括：

```text
Lab 13 kernel_entry reached
Scenario good: no exception-style failure
```

`good` 的作用是建立基线：如果它也不能运行，应先解决环境或工具链问题，而不是分析故障 scenario。

退出 QEMU 通常可按：

```text
Ctrl-a x
```

### Step 2：启动 `bad-pointer` 并连接 GDB

terminal 1：

```bash
make SCENARIO=bad-pointer debug
```

`debug` 使用 QEMU 的 `-s -S`：

```text
-s  在 TCP 1234 端口开启 GDB stub
-S  暂停 CPU，等待 GDB
```

terminal 2 可以直接使用 Makefile shortcut：

```bash
make SCENARIO=bad-pointer gdb
```

也可以手动连接：

```bash
gdb-multiarch build/bad-pointer/kernel.elf
```

```gdb
set architecture riscv:rv64
target remote :1234
```

### Step 3：在坏地址被使用前暂停

先看源码中的关键路径：

```asm
li a0, 0xdeadbeef
bad_pointer_fault_site:
    ld a1, 0(a0)
```

执行前先预测：

1. breakpoint 命中时 `$pc` 应对应哪条指令？
2. 哪个寄存器提供内存地址？
3. 有效地址是多少？
4. 这条 load 如果成功，会把结果写到哪里？

然后验证：

```gdb
break bad_pointer_fault_site
continue
x/i $pc
info registers pc sp ra a0 a1
p/x $a0
```

这些命令分别回答：

```text
x/i $pc                         当前是哪条指令？
info registers pc sp ra a0 a1  相关寄存器是什么状态？
p/x $a0                        load 将使用什么基址？
```

预期证据是：

```text
当前指令是通过 a0 读取内存的 ld
a0 == 0xdeadbeef
有效地址 == 0xdeadbeef + 0
```

`a1` 在执行前仍是旧值。不要把它误认为已经得到 load 结果。

还可以查看附近指令：

```gdb
x/10i $pc
disassemble scenario_bad_pointer
```

现在你已经在不执行故障 load 的情况下证明了根因。

### Step 4：调试 `bad-jump`

结束上一台 QEMU，然后在 terminal 1 启动：

```bash
make SCENARIO=bad-jump debug
```

terminal 2：

```bash
make SCENARIO=bad-jump gdb
```

源码中的关键路径是：

```asm
la t0, unexpected_landing
bad_jump_source:
    jr t0

unexpected_landing:
    ...
```

执行前先预测：

1. `jr t0` 的跳转目标从哪里取得？
2. breakpoint 命中时，`pc` 与 `t0` 应分别指向哪里？
3. 单步执行 `jr t0` 后，新的 `pc` 应当等于什么？

设置 breakpoint 并检查：

```gdb
break bad_jump_source
break unexpected_landing
continue
x/i $pc
info registers pc t0
info symbol $t0
```

这里：

- `$pc` 应指向间接跳转指令；
- `$t0` 应保存 `unexpected_landing` 的地址；
- `info symbol $t0` 让 GDB 尝试把地址重新解释为 symbol。

继续执行：

```gdb
continue
x/i $pc
info registers pc t0
```

第二个 breakpoint 应在 `unexpected_landing` 命中。这个场景的证据链是：

```text
jr t0 使用 t0 作为目标
    -> t0 指向 unexpected_landing
    -> 程序确实在那里停止
```

CPU 正确执行了指令；错误存在于程序选择的控制流目标。

### Step 5：调试 `illegal-instruction`

结束上一台 QEMU，然后启动：

```bash
make SCENARIO=illegal-instruction debug
```

另一个 terminal：

```bash
make SCENARIO=illegal-instruction gdb
```

关键源码：

```asm
illegal_instruction_site:
    .word 0xffffffff
```

设置 breakpoint：

```gdb
break illegal_instruction_site
continue
x/i $pc
info registers pc sp ra
x/4wx $pc
```

`x/4wx $pc` 中：

```text
x  examine memory
/4 显示 4 个单位
w  每个单位是 4-byte word
x  使用 hexadecimal 格式
```

你应在 `$pc` 位置看到包含 `0xffffffff` 的原始 word。不同 GDB 版本可能以不同文本显示无法识别的指令，因此原始 bytes/word 是更直接的证据。

如果希望补充观察 machine CSR，可以先记录：

```gdb
info registers mtvec mcause mepc mtval
```

但不要假设继续执行非法指令后，目标一定会稳定停在方便观察的位置。本实验尚未安装 trap handler。

### Step 6：使用 QEMU log 补充 exception 证据

当执行 exception 后原始现场不容易恢复时，可以使用：

```bash
make SCENARIO=illegal-instruction log
```

日志写入：

```text
build/illegal-instruction/qemu.log
```

该目标启用：

```text
-d int,cpu_reset
```

它让 QEMU 记录 interrupt/exception 和 CPU reset 相关信息。日志适合补充“执行后发生了什么”，而 GDB breakpoint 更适合回答“执行前为什么会发生”。

### Step 7：必要时检查栈，但不要机械使用

```gdb
x/32gx $sp
```

含义：

```text
x    examine memory
/32  显示 32 个单位
g    每个单位为 8 bytes
x    hexadecimal 格式
$sp  从当前栈顶开始
```

本实验的三个主要问题都不由栈直接引起，所以检查栈是辅助动作。好的调试习惯不是每次执行尽可能多的命令，而是根据当前指令选择最相关的证据。

---

## `start.S` 中还会遇到的 assembler directive

这些以 `.` 开头的语句主要告诉 assembler 如何组织程序，不是 CPU 逐条执行的指令：

```text
.equ       定义汇编期常量
.section   选择接下来内容所属的 ELF section
.globl     导出 symbol，使 linker/GDB 可以找到它
.word      放入一个 32-bit 原始值
.asciz     放入以 zero byte 结尾的字符串
.align     对齐接下来的地址
.skip      预留指定数量的 bytes
```

本实验使用：

```asm
.section .rodata
```

保存只读字符串；使用：

```asm
.section .bss
.align 4
stack:
    .skip 4096
stack_top:
```

预留 4096 bytes 作为栈。`stack_top` 标记预留区域末端，因为 RISC-V 栈通常向较低地址增长。

`#if`、`#elif`、`#endif` 则来自 C preprocessor。因为文件名是大写 `.S`，GCC 会先进行预处理，再交给 assembler。Makefile 通过：

```text
-DSCENARIO_ID=...
```

选择最终编译进 `kernel_entry` 的 scenario 调用。

---

## 你现在应该能够解释

1. **为什么只看到 `0xdeadbeef` 还不能证明 bug？**

   因为寄存器中的值只有结合使用它的指令才有意义。`ld a1, 0(a0)` 证明 `a0` 被当作 load 地址。

2. **`ld a1, 0(a0)` 中哪个寄存器提供地址，哪个接收结果？**

   `a0` 提供基址，`a1` 接收读取结果。

3. **看到 `jr t0` 后为什么应该检查 `t0`？**

   因为它是间接跳转，下一条指令地址来自 `t0`。

4. **`bad-jump` 为什么不一定产生 exception？**

   因为目标可以是有效的可执行地址，只是它不符合程序原本期望的控制流。

5. **`.word 0xffffffff` 与坏指针有什么区别？**

   坏指针让合法 load 指令访问不合理地址；`.word 0xffffffff` 让 CPU 尝试译码一个非法指令位模式。

6. **为什么优先在故障指令执行前检查现场？**

   因为本实验没有完整 trap handler。执行后控制流可能离开原位置，使最直接的输入值和指令关系更难观察。

7. **怎样显示当前 RISC-V 指令？**

   ```gdb
   x/i $pc
   ```

8. **怎样显示所有寄存器？**

   ```gdb
   info registers
   ```

9. **应该每次都先查看栈吗？**

   不应该机械地这样做。先读当前指令，再检查它实际使用的寄存器；只有怀疑调用、返回或栈数据时，栈才是主要证据。

## 最后记住

```text
不要从“程序坏了”直接跳到猜测。

先找 pc，
再读当前指令，
再检查这条指令真正使用的寄存器。
```
