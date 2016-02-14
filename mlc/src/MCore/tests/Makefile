# GNU Makefile for MLC++
#  Written by Greg Snyder, Sat Dec 24  1994

include ../../Makefile.config

DIRNAME = MCore

LIBMPREFIX = "$$LIBPREFIX"
LIBNAME = ${MLCDIR}/src/lib/${LIBMPREFIX}lib${DIRNAME}.a

REPMPREFIX = "$$REPPREFIX"
REPNAME =  ./${REPMPREFIX}repository

# All libs would be: -lCCg++ -lG -lL -lm
LINKLIBS = -lMCore -lm

CC_SRCS = \
t_Array.c           t_MLCStream.c       t_RandCharArray.c   t_error2.c\
t_Array2.c          t_MOption.c         t_StatData.c        t_get_env.c\
t_BoolArray.c       t_MRandom.c         t_UnivHashTable.c   \
t_DblLinkList.c     t_MString.c         t_centerline.c      \
t_DynamicArray.c    t_MStringRC.c       t_checkstream.c     t_mach.c\
t_LogOptions.c      t_MaxArray.c        t_error.c           t_mlcIO.c\
t_MEnum.c           t_MinArray.c        t_error1.c          t_safe_new.c


RCS_FILES = ${CC_SRCS} Makefile.${DIRNAME}.tests t_TableCat.c


# What to depend on for all
CC_DEP_SRCS = ${CC_SRCS}   
CC_ALL_SRCS = ${CC_DEP_SRCS}

CC_OBJS = ${CC_SRCS:.c=.o}
CC_DEP_OBJS = ${CC_DEP_SRCS:.c=.o}
CC_DEP_BINS = ${CC_DEP_SRCS:.c=}
CC_ALL_OBJS = ${CC_ALL_SRCS:.c=.o}
CC_ALL_BINS = ${CC_ALL_SRCS:.c=}

All: all

include ../../tmakefile.common

# Specific rules for just this directory
