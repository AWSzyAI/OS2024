NAME := $(shell basename $(PWD))
export MODULE := M2
export LD_LIBRARY_PATH := $(PWD)
all: $(NAME)-64.so $(NAME)-32.so
CFLAGS += -U_FORTIFY_SOURCE -DLOCAL_MACHINE

include ../Makefile
