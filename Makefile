#
# pem 2022-03-05
#
#

CC=gcc -std=c99

CCOPTS=-pedantic -Wall -Werror

#CFLAGS=-g -DDEBUG $(CCOPTS)
CFLAGS=-O $(CCOPTS)

PROG=sudoku

SRC=sudoku.c

OBJ=$(SRC:%.c=%.o)

all:	$(PROG)

$(PROG):	$(OBJ)

clean:
	$(RM) $(OBJ) core

cleanall:	clean
	$(RM) $(PROG) $(LIB) make.deps

make.deps:
	gcc -MM $(CFLAGS) $(SRC) > make.deps

include make.deps
