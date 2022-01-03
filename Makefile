
run: header serial assembly.bin
	./generate bad_header.bin
	./serial /dev/ttyUSB0 bad_header.bin

header:
	gcc -o generate header.c

serial: serial.c
	gcc -o serial serial.c	

assembly.bin: assembly.S
	arm-none-eabi-as -g -mthumb-interwork -mcpu=cortex-m33 -mthumb -o assembly.o assembly.S
	arm-none-eabi-objcopy -O binary assembly.o assembly.bin

.PHONY: clean
clean:
	-rm -f bad_header.bin
	-rm -f generate
	-rm -f assembly.o
	-rm -f assembly.bin
	-rm -f serial
