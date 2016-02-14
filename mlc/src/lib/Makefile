# GNU Makefile for MLC++
#  Written by Greg Snyder, Sat Dec 24 1994

# include here is not relative because lib is a logical link
include ${MLCDIR}/src/Makefile.config

DIRNAME = lib

RCS_FILES = Makefile.${DIRNAME}

LIBS = \
libCCg++.a     libMConv.a     libMGraph.a    libML.a        libMTree.a\
libG.a         libMCore.a     libMIB.a       libMNB.a       libMWrapper.a\
libL.a         libMGLD.a      libMInstGen.a  libMTable.a

all notests notempls: FORCE
	@echo " Nothing to be made in lib"

tests : FORCE
	@echo " Nothing to be made in lib tests"

TAGS : FORCE
	@echo " Nothing to be made in lib TAGS"

# lib regenerates the library, without the template instantiations
lib : 
	@echo " Nothing to be made in lib"

checkin:
	${CI} -u ${RCS_FILES}

initial_checkin:
	${RM} -rf ./RCS
	${MKDIR} ./RCS
	${RCS} -q -i -a"mlc" -c"                  " -L \
    -t-"MLC++ Project, Stanford University" ${RCS_FILES}
	${CI} -u ${RCS_FILES}

checkout:
	${CO} ${RCS_FILES}

clean:
	@echo "Nothing to be done for lib clean"

scratch: clean all

distclean:  clean # used to bring the system back to as pristine as possible a state
	@echo "Deleting all .o and .a files"
	${RM} -f *.o *.a core *~ dummy_file_name
	@echo "Deleting RCS files"
	${RM} -rf ./RCS


install: initial_checkin # just does the install stuff for this directory. Go up two dirs and
# "make install" there to do a full installation

FORCE:
