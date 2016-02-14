#!/bin/sh
for i in ./*.[0-9];
do
    rman -f html -r '%s.%s.html' $i > $i.html
done
