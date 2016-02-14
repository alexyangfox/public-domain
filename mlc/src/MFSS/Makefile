# GNU Makefile for MLC++
#  Written by Greg Snyder, Sat Dec 24 1994

include ../Makefile.config

DIRNAME = MFSS


LINKLIBS = -lMFSS -lMWrapper -lMInd -lMTrans -lMTree -lMGraph -lML -lMCore -lCCg++ -lG -lL -lm 

CC_SRCS = \
FSSInducer.c     FSSState.c       SANode.c         sim_anneal.c \
ProjectCat.c	 ProjectInd.c	  search_ind.c                  \
CompState.c      AccEstState.c    SearchInducer.c               \
OrderState.c     OrderFSSState.c  OrderFSSInd.c			\
C45APState.c     C45APInducer.c   DiscState.c      DiscSearchInd.c \
CascadeCat.c     TableCasInd.c    WeightSearchInd.c \
WeightState.c

# actually templates, so don't compile them (CompState.c has both)
CC_TEMPL_SRCS = \
SSSearch.c       HCSearch.c       State.c          StateSpace.c\
SASearch.c       BFSearch.c       CompState.c


#  The file(s) which depend on all of CC_TEMPL_SRCS, and forces instatiations
TEMPLATE_SRCS = ${DIRNAME}Templates.c
TEMPLATE_OBJS = ${TEMPLATE_SRCS:.c=.o}
TEMPLATE_BINS = ${TEMPLATE_SRCS:.c=}

RCS_FILES = ${CC_SRCS} ${CC_TEMPL_SRCS} ${TEMPLATE_SRCS} Makefile.${DIRNAME}

# What to depend on for all
CC_DEP_SRCS = ${CC_SRCS} 
CC_ALL_SRCS = ${CC_DEP_SRCS} ${CC_TEMPL_SRCS}

CC_OBJS = ${CC_SRCS:.c=.o}
CC_DEP_OBJS = ${CC_DEP_SRCS:.c=.o}
CC_ALL_OBJS = ${CC_ALL_SRCS:.c=.o}

H_FILES_SRC=${CC_SRCS:.c=.h}
H_FILES_TMPL=${CC_TMPL_SRCS}

All: all

review:
	( codeReview MFSS.ps ${CC_SRCS} ${CC_TMPL_SRCS} )
	( cd /u/mlc/inc; codeReview MFSSh.ps ${H_FILES_SRC} ${H_FILES_TMPL})

include ../makefile.common

# Specific rules for just this directory


