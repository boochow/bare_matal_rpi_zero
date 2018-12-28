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
ifeq ($(DEBUG), 1)
COPT = -O0 -gdwarf-2
endif
CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib $(CFLAGS_ARM1176JZF-S) $(COPT)

LDFLAGS = -nostdlib -T rpi.ld -Map=$@.map --cref
LIBS = 
LIBGCC != $(CC) -print-file-name=libgcc.a

SRC_C = \
	main.c \

SRC_S = \

OBJ = $(SRC_C:.c=.o) $(SRC_S:.s=.o)

all: kernel.img

deploy: kernel.img
#set cp command destination to your SD card reader
	cp kernel.img /media/user/4AB2-BF68/

#kernel.img: kernel.elf
#	$(OBJCOPY) -O binary $< $@

kernel.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS) $(LIBGCC)
	$(SIZE) $@

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
