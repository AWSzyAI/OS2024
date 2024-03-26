all: sum
TLIB_PATH := /home/ziyan/Lessons/os-workbench/thread-lib

CFLAGS := -O2 -I$(TLIB_PATH)

sum: sum.c Makefile
	gcc $(CFLAGS) -o $@ $<

clean:
	rm -f sum
