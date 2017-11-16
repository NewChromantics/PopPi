# gr: brew build doesn't boot :/
#ARMPATH ?= /usr/local/Cellar/gcc-arm-none-eabi/20160928
#ARMLIBPATH ?= $(ARMPATH)/lib/gcc/arm-none-eabi/5.4.1/armv7-m
ARMPATH ?= /Users/graham/devkitPro/devkitARM
ARMLIBPATH ?= $(ARMPATH)/lib/gcc/arm-none-eabi/7.1.0

ARMGNU ?= $(ARMPATH)/bin/arm-none-eabi-

TARGET = kernel.img
PROJECTNAME = PopPi

COPS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding 
CPPOPS = -Wall -O2 -std=c++11 -nostdlib -nostartfiles -ffreestanding -DTARGET_CPP -fno-exceptions
ASMOPS = -Wall 

LINKOPS = -L$(ARMLIBPATH) -lgcc 
#-lfplib
#-lgcc

COBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
CPPOBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
SOBJECTS = $(patsubst %.s, %.o, $(wildcard *.s))
ASMOBJECTS = $(patsubst %.asm, %.o, $(wildcard *.asm))

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


%.o : %.s
	@echo compiling as $<
	$(ARMGNU)as $< -o $@

%.o : %.asm
	@echo compiling as $<
	$(ARMGNU)nasm -f elf $<

%.o : %.c
	@echo compiling gcc $<
	$(ARMGNU)gcc $(COPS) -c $< -o $@

%.o : %.cpp
	@echo compiling g++ $<
	$(ARMGNU)g++ $(CPPOPS) -c $< -o $@


$(TARGET) : loader $(SOBJECTS) $(ASMOBJECTS) $(COBJECTS) $(CPPOBJECTS)
	@echo $(SOBJECTS)
	@echo $(ASMOBJECTS)
	@echo $(COBJECTS)
	@echo $(CPPOBJECTS)
	$(ARMGNU)ld $(SOBJECTS) $(ASMOBJECTS) $(COBJECTS) $(CPPOBJECTS) $(LINKOPS) -T loader -o $(PROJECTNAME).elf
	#$(ARMGNU)objdump -D $(PROJECTNAME).elf > $(PROJECTNAME).list
	#$(ARMGNU)objcopy $(PROJECTNAME).elf -O ihex $(PROJECTNAME).hex
	$(ARMGNU)objcopy $(PROJECTNAME).elf -O binary $(TARGET)
	cp $(TARGET) /Volumes/boot/
