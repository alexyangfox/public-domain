#!/bin/bash

# Creates a FreeBSD source tree.

gen1=./create_generated_tree.sh

oldtgtdir=nobuildfiles # must be the same as in $gen1
test -d ${oldtgtdir} || {
    $gen1 || exit
}
test -d ${oldtgtdir} || {
    echo "Missing required directory (output from $gen1): $oldtgtdir"
    exit 127
}

version="$(./libs11n-config --version)"
tgtname="libs11n-${version}-src-freebsd"
tgtdir=${tgtname}

test -d $tgtdir && rm -fr $tgtdir
cp -rp $oldtgtdir $tgtdir

rm $tgtdir/src/Makefile*
cp -vp contrib/build/Makefile.freebsd $tgtdir/src/Makefile

tarball=${tgtname}.tar.bz2
tar cjf ${tarball} ${tgtdir}

echo "Done. Tarball="
ls -la ${tarball}
