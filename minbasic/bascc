#!/bin/sh

libdir=${MINBASIC_DIR:-/u/bin}

if [ "x$1" = "x" -o "x$3" != "x" ]; then
	echo "Usage: bascc file.bas [outfile]"
	exit 1
fi

if [ ! -f "$1" ]; then
	echo "bascc: $1: no such file"
	exit 1
fi

tmp=/tmp/basc$$.c
if [ "x$2" = "x" ]; then
	out=`dirname $1`/`basename $1 .bas`
else
	out="$2"
fi

if [ "$1" = "$out" ]; then
	echo "bascc: $1: source = object (missing .bas suffix?)"
	exit 1
fi

bascomp $1 $tmp $libdir/bascomp.rt && cc -o $out $tmp && rm $tmp
