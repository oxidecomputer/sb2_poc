
all: assembly.bin main.c
	gcc -o generate main.c

assembly.bin: assembly.S
	arm-none-eabi-as -g -mthumb-interwork -mcpu=cortex-m33 -mthumb -o assembly.o assembly.S
	arm-none-eabi-objcopy -O binary assembly.o assembly.bin

.PHONY: clean
clean:
	-rm generate
	-rm assembly.o
	-rm assembly.bin 
