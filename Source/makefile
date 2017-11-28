# gr: brew build doesn't boot :/
#ARMPATH ?= /usr/local/Cellar/gcc-arm-none-eabi/20160928
#ARMLIBPATH ?= $(ARMPATH)/lib/gcc/arm-none-eabi/5.4.1/armv7-m
ARMPATH ?= /Users/graham/devkitPro/devkitARM
ARMLIBPATH ?= $(ARMPATH)/lib/gcc/arm-none-eabi/7.1.0

ARMGNU ?= $(ARMPATH)/bin/arm-none-eabi-

TARGET = kernel.img
PROJECTNAME = PopPi

COPS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding 
CPPOPS = -Wall -O2 -std=c++11 -nostdlib -nostartfiles -ffreestanding -DTARGET_CPP -fno-exceptions -fno-rtti
QPUASMOPS = --exportentry=false

LINKOPS = -L$(ARMLIBPATH) -lgcc 
#-lfplib
#-lgcc

COBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
CPPOBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
SOBJECTS = $(patsubst %.s, %.o, $(wildcard *.s))
QPUOBJECTS = $(patsubst %.qpuasm, %.qpuasm.h, $(wildcard *.qpuasm))

rebuildandupload : rebuild
	@echo Uploading...
#curl -v -F file=@$(TARGET) http://flashair.local/upload.cgi
	curl -v -F file=@$(TARGET) http://192.168.0.19/upload.cgi

rebuild : clean build

build : $(TARGET)

clean :
	rm -f *.o
	rm -f *.bin
	rm -f *.hex
	rm -f *.elf
	rm -f *.list
	rm -f *.img
	rm -f *.bc
	rm -f *.clang.s
	rm -f *.qpuasm.h

%.qpuasm.h : %.qpuasm
	@echo compiling $< qpu assembler to $@


	node Videocoreiv-qpu/qpu-tutorial/qpuasm.js $(QPUASMOPS) --exportarray=$* --in $< > $@

%.o : %.s
	@echo compiling as $<
	$(ARMGNU)as $< -o $@

%.o : %.c
	@echo compiling gcc $<
	$(ARMGNU)gcc $(COPS) -c $< -o $@

%.o : %.cpp
	@echo compiling g++ $<
	$(ARMGNU)g++ $(CPPOPS) -c $< -o $@


$(TARGET) : loader $(QPUOBJECTS) $(SOBJECTS) $(COBJECTS) $(CPPOBJECTS)
	@echo $(QPUOBJECTS)
	@echo $(SOBJECTS)
	@echo $(COBJECTS)
	@echo $(CPPOBJECTS)
	$(ARMGNU)ld $(SOBJECTS) $(COBJECTS) $(CPPOBJECTS) $(LINKOPS) -T loader -o $(PROJECTNAME).elf
	#$(ARMGNU)objdump -D $(PROJECTNAME).elf > $(PROJECTNAME).list
	#$(ARMGNU)objcopy $(PROJECTNAME).elf -O ihex $(PROJECTNAME).hex
	$(ARMGNU)objcopy $(PROJECTNAME).elf -O binary $(TARGET)
