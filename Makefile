RISCVC=riscv32-unknown-elf-gcc
RISCVCXX=riscv32-unknown-elf-g++
RISCVLD=riscv32-unknown-elf-ld
RISCVOBJCOPY=riscv32-unknown-elf-objcopy
RISCVOBJDUMP=riscv32-unknown-elf-objdump
SRC= tests/*.S tests/*.cpp 

# 使用前请先make clean

all:cpupl monitor disk.img Makefile kernel.bin boot.bin
	make -C boot
	make -C tests
cpupl:cpupl.cpp device.hpp
	g++ -o $@ $< -lpthread -Wall
monitor:monitor.cpp
	g++ -o $@ $^
disk.img:kernel.bin boot.bin
	if [ ! -e  disk.img ]; then dd if=/dev/zero of=disk.img bs=1048576 count=256; fi
	dd if=boot.bin of=disk.img bs=1024 count=4
	dd if=kernel.bin of=disk.img bs=1024 count=8192 seek=4
boot.bin:
	make -C boot
	mv boot/boot.bin .
kernel.bin:
	make -C tests
	mv tests/kernel.bin .
moni:monitor
	./monitor
pupl:cpupl
	./cpupl disk.img
clean:
	make clean -C boot
	make clean -C tests 
	rm -rf *.o *.bin cpupl monitor reass.S *.elf