#!/bin/bash
app=./unittests
make ${app} || rm -f ${app}
test -x $app || {
    echo "${app} not executable."
    exit 3
}

log=${app}.valgrind
valgrind --log-fd=3 \
    --leak-check=full -v \
    --show-reachable=yes \
    ${app} 3>${log}
x=$?
grep malloc/free ${log} && {
    echo "Details are in [${log}]"
}
exit $x
