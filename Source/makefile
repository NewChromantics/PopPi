
ARMGNU ?= arm-none-eabi
TARGET = kernel.img
PROJECTNAME = PopPi
COPS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding 
CPPOPS = -Wall -O2  -nostartfiles -ffreestanding -fno-exceptions
COBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
CPPOBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
SOBJECTS = $(patsubst %.s, %.o, $(wildcard *.s))

gcc : $(TARGET)

all : gcc clang

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
	$(ARMGNU)-as $< -o $@

%.o : %.cpp
	$(ARMGNU)-g++ $(CPPOPS) -c $< -o $@

%.o : %.c
	$(ARMGNU)-gcc $(COPS) -c $< -o $@


$(TARGET) : loader $(COBJECTS) $(CPPOBJECTS) $(SOBJECTS)
	@echo $(OBJECTS)
	$(ARMGNU)-ld $(COBJECTS) $(CPPOBJECTS) $(SOBJECTS) -T loader -o $(PROJECTNAME).elf
	$(ARMGNU)-objdump -D $(PROJECTNAME).elf > $(PROJECTNAME).list
	$(ARMGNU)-objcopy $(PROJECTNAME).elf -O ihex $(PROJECTNAME).hex
	$(ARMGNU)-objcopy $(PROJECTNAME).elf -O binary $(TARGET)

