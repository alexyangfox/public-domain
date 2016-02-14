# GNU Makefile for MLC++
#  Written by Greg Snyder, Sat Dec 24  1994

include ../../Makefile.config

DIRNAME = MInstGen


# All libs would be: -lCCg++ -lG -lL -lm
LINKLIBS = -lMInstGen -lMWrapper -lMFSS -lMTrans -lMGraph -lMInd -lMTree -lML -lMCore -lCCg++ -lG -lL -lm

CC_SRCS = \
t_LabInstGen.c  


RCS_FILES = ${CC_SRCS} Makefile.${DIRNAME}.tests t_LIGenFunct.c  


# What to depend on for all
CC_DEP_SRCS = ${CC_SRCS}   # could put t_init.o here first instead of on all:
CC_ALL_SRCS = ${CC_DEP_SRCS}

CC_OBJS = ${CC_SRCS:.c=.o}
CC_DEP_OBJS = ${CC_DEP_SRCS:.c=.o}
CC_DEP_BINS = ${CC_DEP_SRCS:.c=}
CC_ALL_OBJS = ${CC_ALL_SRCS:.c=.o}
CC_ALL_BINS = ${CC_ALL_SRCS:.c=}

All: all

include ../../tmakefile.common

# Specific rules for just this directory
