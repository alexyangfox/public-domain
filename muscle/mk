#!/bin/bash
# profile -pg on compile & link
CPPNames='accmx addseqs align alignmaf alignmasked alignseqtomsa aligntwomsas alnseq blosum70c bwdfull bwdswcrf cartoons codon colprobs cons fasta fastglob fwdbwd fwdbwdfull fwdbwdsw fwdfull fwdswcrf getunsegs guidetree inverts iterfb loglocalaln loglocalalnaln loglocalalnx logmaccs logmsa models msacartoon multilocal multisw multiswx mus4_main mx myutils nucmx params pccrfmx pcparamsda pcrnamx postproc profile progalign prune refine regex repeats rowprobs self selfaln seqdb seqweights simmx sparsemx spps substmx sw swaff testsw tree valcov viterbi xlat'
ObjNames='accmx.o addseqs.o align.o alignmaf.o alignmasked.o alignseqtomsa.o aligntwomsas.o alnseq.o blosum70c.o bwdfull.o bwdswcrf.o cartoons.o codon.o colprobs.o cons.o fasta.o fastglob.o fwdbwd.o fwdbwdfull.o fwdbwdsw.o fwdfull.o fwdswcrf.o getunsegs.o guidetree.o inverts.o iterfb.o loglocalaln.o loglocalalnaln.o loglocalalnx.o logmaccs.o logmsa.o models.o msacartoon.o multilocal.o multisw.o multiswx.o mus4_main.o mx.o myutils.o nucmx.o params.o pccrfmx.o pcparamsda.o pcrnamx.o postproc.o profile.o progalign.o prune.o refine.o regex.o repeats.o rowprobs.o self.o selfaln.o seqdb.o seqweights.o simmx.o sparsemx.o spps.o substmx.o sw.o swaff.o testsw.o tree.o valcov.o viterbi.o xlat.o'

rm -f *.o mus4.make.stdout.txt mus4.make.stderr.txt
for CPPName in $CPPNames
do
  echo $CPPName >> /dev/tty
  g++ -c -O3 -DNDEBUG=1 $CPPName.cpp -o $CPPName.o  >> mus4.make.stdout.txt 2>> mus4.make.stderr.txt
done

g++ -g -o mus4 $ObjNames  >> mus4.make.stdout.txt 2>> mus4.make.stderr.txt
tail mus4.make.stderr.txt

strip mus4
ls -lh mus4
sum mus4
