#
# Makefile for Menu Program
#

CC_PTHREAD_FLAGS			 = -lpthread
CC_FLAGS               = -c 
CC_OUTPUT_FLAGS				 = -o
CC                     = gcc
RM                     = rm
RM_FLAGS               = -f

TARGET  =   test
OBJS    =   linktable.o  menu.o test_exec.o
HELLP_TGR = hello
HELLP_OBJ = hello.o

PWD						= $(shell pwd)
CLANG					= clang
CLANG_FLAGS		= $(CC_FLAGS) -emit-ast
ASTS					= linktable.ast  menu.ast test_exec.ast


all:	$(OBJS) $(HELLP_OBJ) $(ASTS)
	$(CC) $(CC_OUTPUT_FLAGS) $(TARGET) $(OBJS) 
	$(CC) $(CC_OUTPUT_FLAGS) $(HELLP_TGR) $(HELLP_OBJ)
	$(shell ls $(ASTS) | sed "s:^:$(PWD)/: " > astList.txt)

%.o %.ast: %.c
	$(CC) $(CC_FLAGS) -o $@ $<
	$(CLANG) $(CLANG_FLAGS) $<

clean:
	$(RM) $(RM_FLAGS)  *.o $(TARGET) $(HELLP_TGR) *.ast *.bak astList.txt
