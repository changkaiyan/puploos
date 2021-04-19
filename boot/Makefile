RISCVC=riscv32-unknown-elf-gcc
RISCVCXX=riscv32-unknown-elf-g++
RISCVLD=riscv32-unknown-elf-ld
RISCVOBJCOPY=riscv32-unknown-elf-objcopy
RISCVOBJDUMP=riscv32-unknown-elf-objdump
OBJ=boot.o bootsec.o
SRCBOOT=boot.S *.c
all:boot.bin reass.S

boot.bin:boot.elf
	${RISCVOBJCOPY} -O binary -S $^ $@
reass.S:boot.elf
	${RISCVOBJDUMP} -S $^ > $@
boot.elf:*.o 
	${RISCVLD} -Tbootmap.lds -e_start ${OBJ} -o $@
*.o:${SRCBOOT}
	${RISCVC} -c -g -static  -Wall -march=rv32g $^
clean:
	rm -rf *.o *.bin *.elf reass.S