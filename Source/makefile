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
	$(ARMGNU)-as $< -o $@

periph.o : periph.c
	$(ARMGNU)-gcc $(COPS) -c $< -o $@

%.o : %.cpp
	$(ARMGNU)-g++ $(CPPOPS) -c $< -o $@


$(TARGET) : loader $(SOBJECTS) $(COBJECTS) $(CPPOBJECTS)
	@echo $(SOBJECTS)
	@echo $(COBJECTS)
	@echo $(CPPOBJECTS)
	$(ARMGNU)-ld $(SOBJECTS) $(COBJECTS) $(CPPOBJECTS) -T loader -o $(PROJECTNAME).elf
	$(ARMGNU)-objdump -D $(PROJECTNAME).elf > $(PROJECTNAME).list
	$(ARMGNU)-objcopy $(PROJECTNAME).elf -O ihex $(PROJECTNAME).hex
	$(ARMGNU)-objcopy $(PROJECTNAME).elf -O binary $(TARGET)
