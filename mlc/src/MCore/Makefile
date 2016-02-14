# GNU Makefile for MLC++
#  Written by Greg Snyder, Sat Dec 24 1994

include ../Makefile.config

DIRNAME = MCore

LINKLIBS = -lMCore -lm

CC_SRCS = \
StatData.c         machine.c \
MEnum.c            \
BoolArray.c        MLCStream.c        basicCore.c        mlcIO.c\
MOption.c          centerline.c       random.c\
MRandom.c          checkstream.c      safe_new.c\
MString.c          error.c            \
GetOption.c        fatal_abort.c\
get_env.c\
LogOptions.c       RandCharArray.c    


# actually templates, so don't compile them

CC_TEMPL_SRCS = \
Array.c          DynamicArray.c   MaxArray.c       sortCompare.c\
Array2.c         GenPix.c         MinArray.c\
DblLinkList.c    HashTable.c      UnivHashTable.c


#  The file(s) which depend on all of CC_TEMPL_SRCS, and forces instatiations

TEMPLATE_SRCS = ${DIRNAME}Templates.c
TEMPLATE_OBJS = ${TEMPLATE_SRCS:.c=.o}
TEMPLATE_BINS = ${TEMPLATE_SRCS:.c=}

RCS_FILES = ${CC_SRCS} ${CC_TEMPL_SRCS} ${TEMPLATE_SRCS} 

# What to depend on for all

CC_DEP_SRCS = ${CC_SRCS}
CC_ALL_SRCS = ${CC_DEP_SRCS} ${CC_TEMPL_SRCS}

CC_OBJS = ${CC_SRCS:.c=.o}
CC_DEP_OBJS = ${CC_DEP_SRCS:.c=.o}
CC_ALL_OBJS = ${CC_ALL_SRCS:.c=.o}

All: all

include ../makefile.common

# Specific rules for just this directory

centerline.o : centerline.c
	@${RM} -f centerline.o
#       Note no CCFLAGS to cc because it doesn't recognize +k
	@${CLCC} -c $<
	@${AR} cr ${LIBNAME} $@

random.o : random.c
	@${RM} -f random.o
	@echo "random.c"
#       Note no CCFLAGS to cc because it doesn't recognize +k
	@${CLCC} -c $<
	@${AR} cr ${LIBNAME} $@
