# GNU Makefile for MLC++
#  Written by Greg Snyder, Sat Dec 24  1994

include ../../Makefile.config

DIRNAME = ML


# All libs would be: -lCCg++ -lG -lL -lm
LINKLIBS = -lML -lMCore -lCCg++ -lG -lL -lm

CC_SRCS = \
t_AttrCat.c         t_CatTestResult.c   t_DisplayPref.c     t_PartialOrder.c\
t_Attribute.c       t_Categorizer.c     t_Inducer.c         t_Schema.c\
t_AugCategory.c     t_ConstCat.c        t_InstList.c        \
t_BadCat.c          t_ConstInducer.c    t_Instance.c        t_ThresholdCat.c\
t_BagCounters.c	    t_InstanceHash.c    t_entropy.c         t_AttrEqCat.c \
t_BagFeature.c      t_MEnum.c           t_CtrInstList.c     t_NullInducer.c \
t_AccData.c



RCS_FILES = ${CC_SRCS} Makefile.${DIRNAME}.tests t_TableCat.c t_BagSet.c \
		t_CtrBag.c t_CtrInducer.c


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
