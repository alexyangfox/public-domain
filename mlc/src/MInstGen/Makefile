# GNU Makefile for MLC++
#  Written by Greg Snyder, Sat Dec 24 1994

include ../Makefile.config

DIRNAME = MInstGen


LINKLIBS = -lMInstGen -lML -lMCore -lCCg++ -lm

CC_SRCS = \
LIGenFunct.c   LabInstGen.c


# actually templates, so don't compile them
CC_TEMPL_SRCS = 

#  The file(s) which depend on all of CC_TEMPL_SRCS, and forces instatiations
TEMPLATE_SRCS = ${DIRNAME}Templates.c
TEMPLATE_OBJS = ${TEMPLATE_SRCS:.c=.o}
TEMPLATE_BINS = ${TEMPLATE_SRCS:.c=}

RCS_FILES = ${CC_SRCS} ${CC_TEMPL_SRCS} ${TEMPLATE_SRCS} \
		Makefile.${DIRNAME} 

# What to depend on for all
CC_DEP_SRCS = ${CC_SRCS} 
CC_ALL_SRCS = ${CC_DEP_SRCS} ${CC_TEMPL_SRCS}

CC_OBJS = ${CC_SRCS:.c=.o}
CC_DEP_OBJS = ${CC_DEP_SRCS:.c=.o}
CC_ALL_OBJS = ${CC_ALL_SRCS:.c=.o}

All: all

include ../makefile.common

# Specific rules for just this directory
