# GNU Makefile for MLC++
#  Written by Greg Snyder, Sat Dec 24  1994

include ../../Makefile.config

DIRNAME = MGLD

# All libs would be: -lCCg++ -lG -lL -lm
LINKLIBS = -lMGLD -lMGraph -lMTree -lML -lMCore -lCCg++ -lG -lL -lm \
		${MLCMOTIFLIBS} ${MLCSOCKETLIBS}

LINKLIBS1 = -lMGLD -lMGraph -lMTree -lML -lMCore -lCCg++ -lG -lL -lm \
		-lXt -lX11 ${MLCSOCKETLIBS}

CC_SRCS = \
t_DiagramMngr.c   t_GLD.c           t_Shape.c


RCS_FILES = ${CC_SRCS} Makefile.${DIRNAME}.tests


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

# On SunOs, the following Makefile rule fails, so we do the stupid rule...
#	@${MLCDIR}/bin/compile_tester $@ $< ${DIRNAME} -DNO_DISPLAY ${LINKLIBS}
t_DiagramMngr : t_DiagramMngr.c
	@echo "Compiling $@"
	@rm -f $@
	@${MLCDIR}/bin/compile_tester $@ t_DiagramMngr.c ${DIRNAME} -DNO_DISPLAY ${LINKLIBS1}
