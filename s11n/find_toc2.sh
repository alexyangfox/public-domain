#!/bin/bash

test x = "x${TOC2_HOME}" && {
    for d in $PWD/toc2 $PWD/../toc2 $HOME/toc2 /usr/share/toc2 /usr/local/share/toc2; do
        test -e $d/sbin/toc2_bootstrap.sh -a -e $d/bin/mkdep-toc2.c && {
            cd $d > /dev/null
            export TOC2_HOME=$PWD
            cd - > /dev/null
            echo "Found TOC2_HOME: ${TOC2_HOME}"
            break
        }
    done
}

test x = "x${TOC2_HOME}" && {
    cat <<EOF
The build tools were not found :(.

You can download toc2 from http://toc.sourceforge.net

After obtaining toc2, do ONE of the following:

1) Set the TOC2_HOME environment variable to the path containing toc2/bin
   and toc2/sbin. For example, in the bash shell one would use:
   export TOC2_HOME=/path/to/toc2

2) Copy or symlink the 'toc2' tree into the s11n source directory OR one
   directory above the s11n tree. If the original is called toc2-VERSION,
   then name the copy/symlink 'toc2'.

EOF
    exit 1
}

echo "Using TOC2_HOME: ${TOC2_HOME}"
