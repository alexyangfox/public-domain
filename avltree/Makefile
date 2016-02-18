# makefile for tree stuff
# vix 24jul87 [stripped down from as makefile]

CFLAGS		=	-O

TRTEST_OBJ	=	t_trtest.o tree.o

all		:	t_trtest

t_trtest	:	$(TRTEST_OBJ)
			cc -o t_trtest $(TRTEST_OBJ)

clean		:	FRC
			rm -f core a.out t_trtest $(TRTEST_OBJ)

FRC		:

tree.o		:	tree.c tree.h vixie.h
t_trtest.o	:	t_trtest.c tree.h vixie.h
