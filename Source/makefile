# armv6 DOES work on pi zero though, as it's not armv7!
# gr: we need libgcc for aeabi float/math functions
# 	so we have generic(?) arm gcc stuff in the /lib folder, and hardware specific libc stuff in armXX/lib/platformXX/ folders!
# this means we can now compile with homebrew stuff!
ARMPATH ?= /usr/local/Cellar/gcc-arm-none-eabi/20160928
#ARMLIBPATH ?= $(ARMPATH)/lib/gcc/arm-none-eabi/5.4.1/armv6-m
#LINKOPS = -L$(ARMLIBPATH) -lgcc -lc
ARMLIBPATH ?= $(ARMPATH)/arm-none-eabi/lib/armv6-m
LINKOPS = -L$(ARMLIBPATH) -lc -lm -lg -L$(ARMPATH)/lib/gcc/arm-none-eabi/5.4.1/armv6-m -lgcc



#ARMLIBPATH ?= $(ARMPATH)/arm-none-eabi/lib/
#LINKOPS = -L$(ARMLIBPATH) -lc -lstdc++

ARMGNU ?= $(ARMPATH)/bin/arm-none-eabi-

TARGET = kernel.img
PROJECTNAME = PopPi

COPS = -Wall -O2 -nostartfiles -ffreestanding
CPPOPS = -Wall -O2 -std=c++11 -nostartfiles -ffreestanding -DTARGET_CPP -fno-exceptions -fno-rtti
QPUASMOPS = --exportentry=false


#-lfplib
#-lgcc

COBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
COBJECTS += duktape-2.3.0/src/duktape.o
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
