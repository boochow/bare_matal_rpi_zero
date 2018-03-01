BUILD ?= build

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
CFLAGS = -Wall -O3 -std=c11 -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -mno-unaligned-access -nostdlib -nostartfiles -nodefaultlibs -ffreestanding -fno-asynchronous-unwind-tables -fomit-frame-pointer

LDFLAGS = -Wl,-gc-sections -Wl,--build-id=none -Wl,-Bdynamic -Wl,-Map,kernel.map -Wl,-T,rpi32.ld
LIBS = -lc -lm -lgcc

SRC_C = \
	main.c \
	rpi-USB.c \
	emb-stdio.c \
	rpi-SmartStart.c \

SRC_S = \
	SmartStart32.s \

OBJ = $(SRC_C:.c=.o) $(SRC_S:.s=.o)

all: kernel.img

deploy: kernel.img
#set cp command destination to your SD card reader
	# cp kernel.img /media/user/4AB2-BF68/

kernel.elf: rpi32.ld $(OBJ) 
	$(ECHO) "LINK $@"
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $@ $(LIBS)
	$(SIZE) $@

.SUFFIXES : .elf .img

.elf.img:
	$(OBJCOPY) -O binary $< $@
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
.S.o:
	$(CC) $(CFLAGS) -c $< -o $@
clean ::
	$(RM) -f *.o *.map *.img *.elf */*. o */*/*. o
	$(RM) -f tags *~
	$(RM) -f $(BUILD)/*
	$(RM) -d $(BUILD)
