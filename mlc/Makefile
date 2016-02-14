# Makefile for all of MLC++
# This Makefile is run everyday by mlcadmin's cron job.
# When using nice, we must either enclose in parenthesis, or use the
#  bourne shell nice (nice -20).  I prefer using Csh.
SHELL=/usr/bin/csh
# .SHELL is for pmake
.SHELL: path=/usr/bin/csh

all: ${MLCDIR}/src/inc ${MLCDIR}/inc/Array.c ${MLCDIR}/src/lib/libCCg++.a \
	${MLCDIR}/bin/dot ${MLCDIR}/bin/dotty ${MLCDIR}/bin/lefty \
	${MLCDIR}/src/util \
	${MLCDIR}/inc/LEDA ${MLCDIR}/src/lib/libG.a \
	${MLCDIR}/inc/LEDA ${MLCDIR}/src/lib/libL.a \
	${MLCDIR}/src/fastlib/libG.a ${MLCDIR}/src/fastlib/libL.a \
	${MLCDIR}/src/fastlib/libCCg++.a 
	(cd ${MLCDIR}/src; nice +20 pmake -k all) |& tee ${MLCDIR}/src/all.log
	(cd ${MLCDIR}/src; nice +20 make TAGS) |& tee -a ${MLCDIR}/src/all.log
	(cd doc; nice +20 make -k) |& tee -a ${MLCDIR}/src/all.log

#	make keywords

${MLCDIR}/bin/dot:
	ln -s ${MLCDIR}/graphviz/bin/dot ${MLCDIR}/bin

${MLCDIR}/bin/dotty:
	ln -s ${MLCDIR}/graphviz/bin/dotty ${MLCDIR}/bin

${MLCDIR}/bin/lefty:
	ln -s ${MLCDIR}/graphviz/bin/lefty ${MLCDIR}/bin

${MLCDIR}/src/lib/libG.a: ${MLCDIR}/leda/libG.a
	ln -s ${MLCDIR}/leda/libG.a ${MLCDIR}/src/lib

${MLCDIR}/src/lib/libL.a: ${MLCDIR}/leda/libL.a
	ln -s ${MLCDIR}/leda/libL.a ${MLCDIR}/src/lib

${MLCDIR}/src/fastlib/libG.a: ${MLCDIR}/leda/libG.a
	ln -s ${MLCDIR}/leda/libG.a ${MLCDIR}/src/fastlib

${MLCDIR}/src/fastlib/libL.a: ${MLCDIR}/leda/libL.a
	ln -s ${MLCDIR}/leda/libL.a ${MLCDIR}/src/fastlib


${MLCDIR}/src/lib/libCCg++.a: ${MLCDIR}/CClibg++/src/libCCg++.a
	ln -s ${MLCDIR}/CClibg++/src/libCCg++.a ${MLCDIR}/src/lib

${MLCDIR}/src/fastlib/libCCg++.a: ${MLCDIR}/CClibg++/src/libCCg++.a
	ln -s ${MLCDIR}/CClibg++/src/libCCg++.a ${MLCDIR}/src/fastlib

${MLCDIR}/CClibg++/src/libCCg++.a: ${MLCDIR}/CClibg++/src/DLList.o
	(cd ${MLCDIR}/CClibg++/src; make)


${MLCDIR}/src/inc:
	ln -s ${MLCDIR}/inc ${MLCDIR}/src

${MLCDIR}/src/util:
	ln -s ${MLCDIR}/util ${MLCDIR}/src


${MLCDIR}/inc/Array.c:
	(cd inc; make relink)

${MLCDIR}/inc/LEDA: leda/incl/LEDA
	(ln -s ${MLCDIR}/leda/incl/LEDA ${MLCDIR}/inc/LEDA)

${MLCDIR}/CClibg++/src/DLList.o:
${MLCDIR}/leda/libG.a:
${MLCDIR}/leda/libL.a:

count:
	@echo "Counting all lines (running countalllines)"
	@countalllines

keywords:
	@echo "Generating keywords for MLC++"
	@rm -f mlc.sap
	@cp mlc.keywords.sap mlc.sap
	@grep '^class' inc/*.h | tr ':;{' '   ' | awk '{print $$3}' >> mlc.sap
	@grep '^struct' inc/*.h | tr ':;{' '   ' | awk '{print $$3}' >> mlc.sap
	@echo "%%" >> mlc.sap


clean:
	@touch ${MLCDIR}/src/scratch.log
	@rm -f ${MLCDIR}/src/scratch.log.old
	@mv ${MLCDIR}/src/scratch.log ${MLCDIR}/src/scratch.log.old
	(cd ${MLCDIR}/src; make -k clean) |& tee ${MLCDIR}/src/scratch.log

# Note that we don't run scratch in doc because things aren't related
#   in a way that scratch is needed.
scratch:
	@touch ${MLCDIR}/src/scratch.log
	@rm -f ${MLCDIR}/src/scratch.log.old
	@mv ${MLCDIR}/src/scratch.log ${MLCDIR}/src/scratch.log.old
	(cd ${MLCDIR}/src; nice +20 make -k scratch) |& tee ${MLCDIR}/src/scratch.log
	(cd ${MLCDIR}/src; nice +20 make TAGS) |& tee -a ${MLCDIR}/src/scratch.log
	(cd doc; nice +20 make -k) |& \
            tee -a ${MLCDIR}/src/scratch.log
	make keywords

FORCE:
