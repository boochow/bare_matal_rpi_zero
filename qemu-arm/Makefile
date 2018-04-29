CROSS_COMPILE = arm-none-eabi-
AS = $(CROSS_COMPILE)as
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size
STRIP = $(CROSS_COMPILE)strip
AR = $(CROSS_COMPILE)ar
ECHO = @echo

INC += -I.
CFLAGS_ARM1176JZF-S = -mabi=aapcs-linux -mcpu=arm1176jzf-s -msoft-float
CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib $(CFLAGS_ARM1176JZF-S) $(COPT)

LDFLAGS = -nostdlib -T rpi.ld -Map=$@.map --cref
LIBS =

SRC_C = \
	main.c \

SRC_S = \

OBJ = $(SRC_C:.c=.o) $(SRC_S:.s=.o)

all: run

kernel.elf: $(OBJ) rpi.ld
	$(ECHO) "LINK $@"
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(SIZE) $@

run: kernel.img
	@qemu-system-arm \
	-kernel $< \
	-append "qemu" \
	-cpu arm1176 \
	-M versatilepb \
	-m 512 \
	-no-reboot \
	-nographic \
	-monitor null \
	-serial stdio \
	-redir tcp:10022::22

.SUFFIXES : .elf .img

.elf.img:
	$(OBJCOPY) -O binary $< $@
.c.o:
	$(CC) $( CFLAGS ) -c $< -o $@
.S.o:
	$(CC) $( CFLAGS ) -c $< -o $@
clean ::
	$(RM) -f *.o *.map *.img *.elf */*. o */*/*. o
	$(RM) -f tags *~
