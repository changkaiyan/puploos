# puploos
PUPLOOS计算机操作系统，用于操作系统入门教学

# requirements

riscv编译工具链：请自行测试riscv32的编译工具链是否正常，使得下面的指令可以运行。

```bash
RISCVC=riscv32-unknown-elf-gcc
RISCVCXX=riscv32-unknown-elf-g++
RISCVLD=riscv32-unknown-elf-ld
RISCVOBJCOPY=riscv32-unknown-elf-objcopy
RISCVOBJDUMP=riscv32-unknown-elf-objdump
```

# 基本使用

`make all`可以构建整个工程，`make clean`用于清空整个工程，`make pupl`用于运行模拟器，`make moni`用于运行监视器
