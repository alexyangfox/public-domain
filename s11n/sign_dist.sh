#!/bin/bash
# Quick hack to generate .sig files for
infiles=$(ls libs11n-*.gz libs11n-*.bz2 libs11n-*.zip libs11n-*.deb 2>/dev/null)

test x = "x${infiles}" && {
    echo "No dist files found."
    exit 127
}

bailout()
{
    err=$1
    shift
    echo "Error: " $@
    exit $err
}

KLUDGE_GPGKEYID="-u 93DA9E0B" # the official s11n.net key for signing downloadables.
for i in $infiles; do
    sig=$i.sig
    test -f $sig && rm -f $sig
    gpg ${KLUDGE_GPGKEYID} --armor --output $sig --detach-sig $i || bailout $? "Signing failed"
    gpg ${KLUDGE_GPGKEYID} --verify $sig $i || bailout $? "Verification failed"
    echo "Signed: $sig"
done
