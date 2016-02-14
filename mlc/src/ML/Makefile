# GNU Makefile for MLC++
#  Written by Greg Snyder, Sat Dec 24 1994

include ../Makefile.config

DIRNAME = ML

LIBMPREFIX = "$$LIBPREFIX"
LIBNAME = ${MLCDIR}/src/lib/${LIBMPREFIX}lib${DIRNAME}.a

REPMPREFIX = "$$REPPREFIX"
REPNAME =  ./${REPMPREFIX}repository

LINKLIBS = -lML -lMCore -lCCg++ -lG -lL -lm 

CC_SRCS = \
TableCat.c DisplayPref.c \
AttrCat.c         BaseInducer.c     CtrInstList.c     PartialOrder.c\
Attribute.c       CatTestResult.c   Inducer.c         Schema.c\
AugCategory.c     Categorizer.c     InstList.c        ThresholdCat.c\
BadCat.c          ConstCat.c        Instance.c        basicML.c\
BagCounters.c     ConstInducer.c    InstanceHash.c    distance.c\
BagFeature.c      CtrBag.c          entropy.c	      stubs.c \
BagSet.c          CtrInducer.c      NullInducer.c     DiscCat.c \
AttrEqCat.c       AccData.c


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
