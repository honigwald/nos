#
# Kurzanleitung
# =============
#
# make		-- Baut den Kernel.
# make all
#
# make install	-- Baut den Kernel und transferiert ihn auf den Server.
# 		   Das Board holt sich diesen Kernel beim nächsten Reset.
#
# make clean	-- Löscht alle erzeugten Dateien.
#


#
# Quellen
#
LSCRIPT = kernel.lds
OBJ  = sys/start.o
OBJ += sys/kernel_main.o
OBJ += sys/interrupt_handler.o
OBJ += sys/interrupt_handler_asm.o
OBJ += sys/trigger.o
OBJ += sys/wait_asm.o
OBJ += sys/wait.o
OBJ += sys/thread_control.o
OBJ += sys/mmu.o
OBJ += sys/thread_control_asm.o
OBJ += sys/channels.o
OBJ += opt/syscalls.o
OBJ += opt/syscalls_asm.o
OBJ += opt/uprintf.o
OBJ += opt/userthread.o
OBJ += opt/uwait.o
OBJ += opt/uwait_asm.o
OBJ += opt/utrigger_asm.o
OBJ += driver/uart.o
OBJ += driver/interrupt_timer.o
OBJ += driver/interrupt_registers.o
OBJ += lib/flags.o
OBJ += lib/printf.o
OBJ += lib/fifo.o
OBJ += lib/stdlib.o

SUBMISSION_FILES = $(shell find . -name "*.[chS]") $(shell find . -name "Makefile*") $(shell find . -name "*.lds")
TUBIT_NAMES = $(shell awk 'NR > 1  {ORS="+"; print prev} {prev=$$1} END { ORS=""; print $$1 }' namen.txt )
#TUBIT_NAMES = $(shell cut -d' ' -f1  namen.txt | tr '\n' '+')

ifeq ($(TUBIT_NAMES), )
    TUBIT_NAMES = "invalid"
    $(error "namen.txt ist fehlerhaft oder leer!")
    INVAL = 1
endif

#
# Konfiguration
#
CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

CFLAGS = -Wall -Wextra -ffreestanding -march=armv7-a -mtune=cortex-a7 -O2
CPPFLAGS = -Iinclude

DEP = $(OBJ:.o=.d)

#
# Regeln
#
.PHONY: all
all: kernel kernel.bin

-include $(DEP)

%.o: %.S
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -o $@ -c $<

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -o $@ -c $<

kernel: $(LSCRIPT) $(OBJ)
	$(LD) -T$(LSCRIPT) -o $@ $(OBJ) $(LIBGCC)

kernel.bin: kernel
	$(OBJCOPY) -Obinary --set-section-flags .bss=contents,alloc,load,data $< $@

kernel.img: kernel.bin
	mkimage -A arm -T standalone -C none -a 0x100000 -d $< $@

.PHONY: install
install: kernel.img
#	cp -v kernel.img /srv/tftp
	arm-install-image $<

.PHONY: clean
clean:
	rm -f kernel kernel.bin kernel.img
	rm -f $(OBJ)
	rm -f $(DEP)

submission:	kernel namen.txt $(SUBMISSION_FILES)
	tar -czf "$(TUBIT_NAMES).tar.gz" $^

