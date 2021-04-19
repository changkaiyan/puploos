RISCVC=riscv32-unknown-elf-gcc
RISCVCXX=riscv32-unknown-elf-g++
RISCVLD=riscv32-unknown-elf-ld
RISCVOBJCOPY=riscv32-unknown-elf-objcopy
RISCVOBJDUMP=riscv32-unknown-elf-objdump
SRC= *.S *.cpp 
TMP=./init.o $(shell find . -maxdepth 1 -name "*.o"|xargs|sed 's/.\/init.o//g')

kernel.bin:kernel.elf reass.s
	${RISCVOBJCOPY} -O binary -S kernel.elf $@
reass.s:kernel.elf
	${RISCVOBJDUMP} -S --debugging $^ > $@

kernel.elf:*.o
	${RISCVLD} -Tkernelmap.lds  -nostdlib -e_start ${TMP} -o $@

*.o:${SRC} # -fno-rtti表示禁止使用动态特性
	${RISCVC} -fno-pic -fno-builtin -c -O0 -g -static -fno-rtti -Wall -march=rv32g $^ 
clean:
	rm -rf *.o *.bin reass.S *.elf