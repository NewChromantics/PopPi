ARMGNU ?= arm-none-eabi

TARGET = kernel.img
PROJECTNAME = PopPi

COPS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding 
CPPOPS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding -fno-exceptions -DTARGET_CPP

COBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
CPPOBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
SOBJECTS = $(patsubst %.s, %.o, $(wildcard *.s))

gcc : $(TARGET)

all : gcc

clean :
	rm -f *.o
	rm -f *.bin
	rm -f *.hex
	rm -f *.elf
	rm -f *.list
	rm -f *.img
	rm -f *.bc
	rm -f *.clang.s


vectors.o : vectors.s
	$(ARMGNU)-as vectors.s -o vectors.o

periph.o : periph.c
	$(ARMGNU)-gcc $(COPS) -c periph.c -o periph.o

%.o : %.cpp
	$(ARMGNU)-g++ $(CPPOPS) -c $< -o $@


$(TARGET) : loader vectors.o periph.o $(CPPOBJECTS)
	@echo $(SOBJECTS)
	@echo $(COBJECTS)
	@echo $(CPPOBJECTS)
	$(ARMGNU)-ld vectors.o periph.o $(CPPOBJECTS) -T loader -o video01.elf
	$(ARMGNU)-objdump -D video01.elf > video01.list
	$(ARMGNU)-objcopy video01.elf -O ihex video01.hex
	$(ARMGNU)-objcopy video01.elf -O binary $(TARGET)
