# GNU Makefile for MLC++
#  Written by Greg Snyder, Sat Dec 24 1994
#  Modified by Ronny Kohavi for SGI.  Nov 1995

include ../src/Makefile.config

# Don't remove these because we usually abort the .verify step.
.PRECIOUS: Inducer AccEst

DIRNAME = util

LINKLIBS = -lMWrapper -lMFSS -lMWrapper -lMTrans -lMGraph -lMInd -lMTree -lML -lMCore -lCCg++ -lG -lL -lm


#STATIC=-Bstatic
STATIC=

CC_SRCS = \
u_Inducer.c	    u_AccEst.c          u_project.c\
u_conv.c            u_info.c	        u_ID3.c \
u_GenCVFiles.c      u_LearnCurve.c      u_discretize.c\
u_C45Tree.c         \
u_convertBuntine.c  \
u_GLD.c             u_Inducer.c         u_convertNames.c \
u_indTest.c

#  The order here matters
CC_BINS = \
Inducer		AccEst		info \
discretize	C45Tree		project		CV		\
ID3		LearnCurve	convertBuntine	\
GLD		conv		convertNames \
indTest		GenCVFiles


# actually templates, so don't compile them
CC_TEMPL_SRCS = 

RCS_FILES = ${CC_SRCS} ${CC_TEMPL_SRCS} Makefile.${DIRNAME} 

# What to depend on for all
CC_DEP_SRCS = ${CC_SRCS}
CC_ALL_SRCS = ${CC_DEP_SRCS} ${CC_TEMPL_SRCS}

CC_OBJS = ${CC_SRCS:.c=.o}
CC_DEP_OBJS = ${CC_DEP_SRCS:.c=}
CC_DEP_BINS = ${CC_BINS}
CC_ALL_OBJS = ${CC_ALL_SRCS:.c=.o}
CC_ALL_BINS = ${CC_DEP_BINS}


All: all

# This rule makes everything:  it is the default rule
all : ${CC_DEP_BINS}

Inducer: u_Inducer.c
	@${MLCDIR}/bin/compile_util  $@ $? ${DIRNAME} ${STATIC} ${LINKLIBS}

AccEst: u_AccEst.c
	@${MLCDIR}/bin/compile_util  $@ $? ${DIRNAME} ${STATIC} ${LINKLIBS}

info: u_info.c
	@${MLCDIR}/bin/compile_util  $@ $? ${DIRNAME} ${STATIC} ${LINKLIBS}

discretize: u_discretize.c
	@${MLCDIR}/bin/compile_util  $@ $? ${DIRNAME} ${STATIC} ${LINKLIBS}

C45Tree: u_C45Tree.c
	@${MLCDIR}/bin/compile_util  $@ $? ${DIRNAME} ${STATIC} ${LINKLIBS}

project: u_project.c
	@${MLCDIR}/bin/compile_util  $@ $? ${DIRNAME} ${STATIC} ${LINKLIBS}

CV: u_CV.c
	@${MLCDIR}/bin/compile_util  $@ $? ${DIRNAME} ${STATIC} ${LINKLIBS}

GenCVFiles: u_GenCVFiles.c
	@${MLCDIR}/bin/compile_util  $@ $? ${DIRNAME} ${STATIC} ${LINKLIBS}

ID3: u_ID3.c
	@${MLCDIR}/bin/compile_util  $@ $? ${DIRNAME} ${STATIC} ${LINKLIBS}

LearnCurve: u_LearnCurve.c
	@${MLCDIR}/bin/compile_util  $@ $? ${DIRNAME} ${STATIC} ${LINKLIBS}

convertBuntine: u_convertBuntine.c
	@${MLCDIR}/bin/compile_util  $@ $? ${DIRNAME} ${STATIC} ${LINKLIBS}

conv: u_conv.c
	@${MLCDIR}/bin/compile_util  $@ $? ${DIRNAME} ${STATIC} ${LINKLIBS}

convertNames: u_convertNames.c
	@${MLCDIR}/bin/compile_util  $@ $? ${DIRNAME} ${STATIC} ${LINKLIBS}

indTest: u_indTest.c
	@${MLCDIR}/bin/compile_util  $@ $? ${DIRNAME} ${STATIC} ${LINKLIBS}

# These are GNU make rules.  Oh, the good makes that exist...
#  the dependency list manually.
# %: u_%.c
#	@echo "Compiling $@ from $<:"
#	@${MLCDIR}/bin/compile_util  $@ $< ${DIRNAME} ${STATIC} ${LINKLIBS}
#
#%: u_%.o
#	@echo "Compiling $@ from $<:"
#	@${MLCDIR}/bin/compile_util  $@ $< ${DIRNAME} ${STATIC} ${LINKLIBS}

GLD: u_GLD.c
	@echo "Compiling $@ from GLD.c:"
	@${MLCDIR}/bin/compile_util  GLD u_GLD.c ${DIRNAME} ${STATIC} -lMGLD -lMInstGen ${LINKLIBS} -lXm -lXt -lX11 $${MLCSOCKETLIBS}


TAGS : ${CC_SRCS}
	@${GENTAGS} $${TAGSFLAGS} ${CC_SRCS}

clean: 
	@echo "Deleting objects and binaries"
	@${RM} -f ${CC_ALL_OBJS} ${CC_ALL_BINS} core
#	@echo "Clearing hdrepository"; 
#	@${RM} -rf ${MLCDIR}/inc/hdrepository
	@echo "Clearing ${LOCALREP}"; 
	@${RM} -rf ${LOCALREP}
	@echo "Creating new ${LOCALREP}"
	@${MKDIR} ${LOCALREP}
	@${CHMOD} g+w ${LOCALREP}

scratch: clean
	@${MAKE} all

notests notempls: all

# Used to force a rule to happen
FORCE:

# Specific rules for just this directory

