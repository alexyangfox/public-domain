#!/bin/sh
if gcc -g -Wall -pedantic *.c -o testbitap && ./testbitap
then
  mkdir libbitap-1.02
  cp -a *.c *.h *.sh index.html libbitap-*
  tar cjf libbitap.tbz libbitap-*
  tar czf libbitap.tgz libbitap-*
  zip -r -m libbitap.zip libbitap-*
fi
