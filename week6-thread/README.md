# Week 6：Kernel Threads and Synchronization

对应 ECE391-L7、L8、L9。建议顺序：

1. [Lab 16：Context Switch](lab16-context-switch/README.md)
2. [Lab 17：Spawn、RR、Lifecycle、Condition](lab17-scheduler-lifecycle/README.md)
3. [Lab 18：Race、Lock、IRQ Critical Section](lab18-race-lock-irq/README.md)

三个实验均使用 QEMU `virt`、RV64 裸机代码与 GDB remote debugging，不使用
Linux `pthread`。

## 能力链

```text
ra/sp/s0-s11 + tp + independent stacks
→ artificial first context + thread_setup
→ ready queue + lifecycle
→ WAITING + condition broadcast
→ lost update + critical section
→ blocking lock + interrupt save/restore
```

## 工具

```bash
sudo apt install qemu-system-misc gcc-riscv64-unknown-elf \
  binutils-riscv64-unknown-elf gdb-multiarch make
```

每个实验支持：

```bash
make MODE=solution run-check
make MODE=exercise
make MODE=solution debug
make MODE=solution gdb
```

运行全部验证：

```bash
bash scripts/verify-week6.sh
```
