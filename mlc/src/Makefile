# GNU Makefile for MLC++:  src
#
# This Makefile is called to make the sources for MLC++.  It recursively makes
# the subdirectories containing the libraries and the tests.
#  By Greg Snyder, revised: 18 Jan 1995
#  Modified by Ronny Kohavi 11 Nov 1995 for SGI and parallel make

include Makefile.config		# All of the configuration stuff

RCS_FILES= 

SRCTARGETS= MCore ML MTree MGraph MInd MTrans MWrapper MFSS MInstGen MGLD 
TARGETS=  inc ${SRCTARGETS} lib util
SRCSUBDIRS= ${SRCTARGETS}
SUBDIRS=  inc ${SRCTARGETS} util

NOTESTTARGETS =  $(TARGETS:=.notests)
NOTEMPLTARGETS =  $(TARGETS:=.notempls)

ALLTARGETS= all notests clean TAGS lib

#  These descend into all directories in the source tree, so be careful!
#  Note that things which depend on a source directory may also descend into
#  the source tree
RECRUSIVETARGETS= all notests notempls clean scratch cleantesters distclean \
	alltc cleantc scrachtc tcreport TAGS 

.NOTPARALLEL:

all: checkEnv $(TARGETS)

checkEnv:
	@../bin/checkEnv


# everything except tests
notests: ${NOTESTTARGETS}

# everything except tests and template instantiations
notempls: ${NOTEMPLTARGETS}


#  Makes all the directories
lib:	${SRCTARGETS}

# This is the rule that makes all in our subdirectories
${TARGETS}: FORCE
	@echo "Making $@..."
	@(cd $@; $(MAKE) -f Makefile $(MFLAGS))


# The rules for making without the tests.  The sed stuff just strips the 
# .notests off of the dependancy name
${NOTESTTARGETS}: FORCE
	@echo "Making $@..."
	@(cd `echo $@ | sed 's/\..*//'`; $(MAKE)  $(MFLAGS) notests)


# The rules for making without the tests or templates.  The sed stuff just 
# strips the .notests off of the dependancy name
${NOTEMPLTARGETS}: FORCE
	@echo "Making $@ notests/notempl..."
	@(cd `echo $@ | sed 's/\..*//'`; $(MAKE)  $(MFLAGS) notempls))


# Note that TAGS don't depend on .h because the names aren't listed
# here.  We might want to add them, but it's not critical.
TAGS : FORCE
	@echo "Removing old TAGS"
	@${RM} -f ${MLCDIR}/src/TAGS
	@echo "Making new TAGS"
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir; $(MAKE) -f Makefile $(MFLAGS) TAGS); done

clean:
	@echo "Removing ${GLOBALREP}"
	@${RM} -rf ${GLOBALREP}
	@echo "Creating new ${GLOBALREP}"
	@${MKDIR} ${GLOBALREP}
	@${CHMOD} g+w ${GLOBALREP}
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir; echo cleaning in $$dir; $(MAKE) -f Makefile $(MFLAGS) clean); done


alltc: FORCE
	@for dir in $(SRCSUBDIRS) ; do \
	  (cd $$dir; $(MAKE) -f Makefile $(MFLAGS) tests.alltc); done

cleantc: FORCE
	@for dir in $(SRCSUBDIRS) ; do \
	  (cd $$dir; $(MAKE) -f Makefile $(MFLAGS) tests.cleantc); done

scratchtc: FORCE
	@for dir in $(SRCSUBDIRS) ; do \
	  (cd $$dir; $(MAKE) -f Makefile $(MFLAGS) tests.scratchtc); done

tcreport: FORCE
	@for dir in $(SRCSUBDIRS) ; do \
	  (cd $$dir; $(MAKE) -f Makefile $(MFLAGS) tests.tcreport); done

cleantesters: FORCE
	@for dir in $(SRCSUBDIRS) ; do \
	  (cd $$dir; $(MAKE) -f Makefile $(MFLAGS) tests.clean); done
	

scratch:  clean
	${MAKE} all

scratchnotests:  clean notests


FORCE:

