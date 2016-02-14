#!/bin/sh
test ! -n "$HLIBC" && test -d ../hlibc && HLIBC=../hlibc
test ! -n "$SRC" && SRC="*.c"
for i in $SRC
do
	echo -n "building: ${i%.c}: "
	if cc -nostdlib -nostdinc -I$HLIBC/include -o ${i%.c} $HLIBC/ctr0.o $i lib/* $HLIBC/hlibc.a 2> ${i%.c}-hlibc.log
	then
		echo "Ok"
		rm ${i%.c}-hlibc.log
	else
		echo "Error"
	fi
done
