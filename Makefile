
all:
	gcc -o generate main.c

.PHONY: clean
clean:
	-rm generate 
