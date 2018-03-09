TOOL_CHAIN = arm-none-eabi-
CC = ${TOOL_CHAIN}gcc
AS = ${TOOL_CHAIN}as
LD = ${TOOL_CHAIN}ld
OBJCOPY = ${TOOL_CHAIN}objcopy
OBJDUMP = $(TOOL_CHAIN)objdump

CFLAGS		:= -Wall -g -fno-builtin -gdwarf-2 -gstrict-dwarf -mcpu=cortex-m3 -mthumb -nostartfiles  --specs=nosys.specs -std=c11 \
				-O0 -Iinclude
LDFLAGS		:= -g

objs := int_vector.o main.o cm3.o os_stdio.o cm3_s.o task.o lib.o event.o sem.o mailbox.o memblock.o

rtos.bin: $(objs)
	${LD} -T rtos.ld -o rtos.elf $^
	${OBJCOPY} -O binary -S rtos.elf $@
	${OBJDUMP} -D -m arm rtos.elf > rtos.dis

run: $(objs)
	${LD} -T rtos.ld -o rtos.elf $^
	${OBJCOPY} -O binary -S rtos.elf rtos.bin
	${OBJDUMP} -D -m arm rtos.elf > rtos.dis
	qemu-system-arm -M lm3s6965evb --kernel rtos.bin -nographic

debug: $(objs)
	${LD} -T rtos.ld -o rtos.elf $^
	${OBJCOPY} -O binary -S rtos.elf rtos.bin
	${OBJDUMP} -D -m arm rtos.elf > rtos.dis
	qemu-system-arm -M lm3s6965evb --kernel rtos.bin -nographic -s -S

%.o:%.c
	${CC} $(CFLAGS) -c -o $@ $<

%.o:%.s
	${CC} $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o *.elf *.bin *.dis
