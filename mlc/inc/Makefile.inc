# GNU Makefile for MLC++
#  Written by Greg Snyder, Sat Dec 24 1994

include ../src/Makefile.config

REPMPREFIX = "$$REPPREFIX"
REPNAME =  ./${REPMPREFIX}repository

CC_SRCS= \
Array.c          DynamicArray.c   MaxArray.c       UnivHashTable.c\
Array2.c         GenPix.c         MinArray.c       sortCompare.c\
DblLinkList.c    HashTable.c      OneR.c	   LazyDTCat.c


CC_HDRS= \
AccEstimator.h          DynamicArray.h          PartialOrder.h\
Array.h                 GLD.h                   ProjBag.h\
Array2.h                GLDPref.h               ProjStats.h\
AttrCat.h               GenPix.h                RDGCat.h\
Attribute.h             GetOption.h             RandCharArray.h\
AugCategory.h           HOODGCIH.h              RefCount.h\
BadCat.h                HOODGInducer.h          RootCatGraph.h\
BagCounters.h           HashTable.h             Schema.h\
BagSet.h                IBCategorizer.h         SchemaRC.h\
BaseInducer.h           IBInducer.h             Shape.h\
BoolArray.h             ID3Inducer.h            StatData.h\
Bootstrap.h             IncrInducer.h           StratifiedCV.h\
C45Inducer.h            Inducer.h               TDDTInducer.h\
CGraph.h                InstList.h              TableCat.h\
CVIncremental.h         Instance.h              TableInducer.h\
CValidator.h            InstanceAndDistance.h   ThresholdCat.h\
CatGraph.h              InstanceHash.h          UnivHashTable.h\
CatTestResult.h         InstanceRC.h            basics.h\
Categorizer.h           LIGenFunct.h            checkstream.h\
ConstCat.h              LabInstGen.h            distance.h\
ConstInducer.h          LearnCurve.h            entropy.h\
CtrBag.h                LogOptions.h            env_inducer.h\
CtrInducer.h            MEnum.h                 error.h\
CtrInstList.h           MLCStream.h             errorUnless.h\
DTCategorizer.h         MOption.h               isocat.h\
DblLinkList.h           MRandom.h               machSVR4.h\
DecisionTree.h          MString.h               machSunOS.h\
DestArray.h             MStringRC.h             mlcIO.h\
Diagram.h               MaxArray.h              random.h\
DiagramMngr.h           MinArray.h              safe_new.h\
DisplayMngr.h           NullInducer.h           sortCompare.h\
DisplayPref.h           OODGInducer.h\
OneRInducer.h		OneR.h


ALL_FILES= ${CC_SRCS} ${CC_HDRS} Makefile.inc
RCS_FILES= ${CC_HDRS} Makefile.inc

all notests notempls: 
	@echo "Nothing to be made here."


TAGS : ${CC_HDRS}
	@${GENTAGS} $${TAGSFLAGS} ${CC_SRCS}

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

relink:
	@echo "Removing .c files."
	@${RM} -f ${CC_SRCS}
	@echo "Re-sym-linking .c files."
	@for dir in  Array.c Array2.c DblLinkList.c DynamicArray.c GenPix.c HashTable.c MaxArray.c MinArray.c UnivHashTable.c sortCompare.c ; do \
		ln -s ${MLCDIR}/src/MCore/$$dir . ; done
	ln -s ${MLCDIR}/src/MTrans/OneR.c .
	ln -s ${MLCDIR}/src/MTree/LazyDTCat.c .

clean: relink
	@echo "Clearing ${REPNAME}"; 
	@${RM} -rf ${REPNAME}
	@echo "Creating new ${REPNAME}"
	@${MKDIR} ${REPNAME}
	@${CHMOD} g+w ${REPNAME}
	@echo "Clearing hdrepository"; 
	@${RM} -rf hdrepository
	@echo "Creating new hdrepository"
	@${MKDIR} hdrepository
	@${CHMOD} g+w hdrepository

distclean:  clean # used to bring the system back to as pristine as possible a state
	@echo "Deleting all .o and .a files"
	${RM} -f *.o *.a core *~ TAGS dummy_file_name
	@echo "Deleting RCS files"
	${RM} -rf ./RCS
	@echo "Deleting repositorys"
	${RM} -rf ./*repository

install: initial_checkin # just does the install stuff for this directory. Go up two dirs and
# "make install" there to do a full installation
	${RM} -rf ${REPNAME}
	${MKDIR} ${REPNAME}
	${CHMOD} g+w ${REPNAME}

	@echo "Clearing hdrepository"; 
	${RM} -rf hdrepository
	@echo "Creating new hdrepository"
	${MKDIR} hdrepository
	${CHMOD} g+w hdrepository
