#!/bin/bash
# A quick hack to generate a Debian package of libs11n. i took most of this
# from Martin Krafft's "The Debian System" book.

DEB_REV=${1-3} # .deb package build/revision number.
DEB_PACKAGE_NAME=libs11n13-dev


if uname -a | grep -i nexenta &>/dev/null; then
# Assume NexentaOS/GnuSolaris:
    DEB_PLATFORM=nexenta
    DEB_ARCH_NAME=solaris-i386
else
    DEB_PLATFORM=${DEB_PLATFORM-ubuntu-gutsy}
    DEB_ARCH_NAME=i386
fi

SRCDIR=${PWD}
test -f ${SRCDIR}/ChangeLog -a -f ${SRCDIR}/include/s11n.net/s11n/s11n_config.hpp || {
    echo "This script must be run from a BUILT copy of the libs11n source tree."
    exit 1
}



DEBDIR=$PWD/deb
DEBPREFIX=/usr
DEBLOCALPREFIX=./${DEBPREFIX}
mkdir -p ${DEBDIR}
mkdir -p ${DEBDIR}${DEBPREFIX}/share/doc/libs11n
     # ^^^^ obligatory kludge: if docs are not gen'd for s11n, the
     # Debian-required copyright file cannot be created.
cd $DEBDIR || {
    echo "Debian dest dir [$DEBDIR] not found. :("
    exit 2
}


test -f usr/bin/libs11n-config || {
    cat <<EOF
REMINDER: you should have configured the tree with:

  ./configure --pure-iso

or:

  ./configure \
      --without-zfstream \
      --without-libexpat \
      --prefix=${DEBPREFIX}

And installed it with:

  make install DESTDIR=${DEBDIR}

EOF
    exit 3
}

rm -fr DEBIAN
mkdir DEBIAN
# TODO: create separate doc package with doxygen and user's guide
find usr -type d -name 'doxygen-*' | xargs rm -fr


S11N_VERSION=$(${DEBLOCALPREFIX}/bin/libs11n-config --version)
S11N_DEB_VERSION=${S11N_VERSION}-${DEB_REV}
DEBFILE=${SRCDIR}/libs11n-${S11N_DEB_VERSION}-dev-${DEB_ARCH_NAME}-${DEB_PLATFORM}.deb
PACKAGE_TIME=$(/bin/date)

rm -f ${DEBFILE}
echo "Creating .deb package [${DEBFILE}]..."

echo "Generating md5 sums..."
find ${DEBLOCALPREFIX} -type f -exec md5sum {} \; > DEBIAN/md5sums

true && {
    echo "Generating Debian-specific files..."
    COPYRIGHT=${DEBLOCALPREFIX}/share/doc/libs11n/copyright
    cat <<EOF > ${COPYRIGHT}
This package was created by stephan beal <stephan@s11n.net>
on ${PACKAGE_TIME}.

The original sources for libs11n can be downloaded from:

http://s11n.net/download/

The author of libs11n disclaims all copyrights: it is released
into the Public Domain. 

EOF
}

true && {
    CHANGELOG=${DEBLOCALPREFIX}/share/doc/libs11n/changelog.gz
    cat <<EOF | gzip -c > ${CHANGELOG}
libs11n ${S11N_DEB_VERSION}; urgency=low

This release has no changes over the core source distribution. It has
simply been Debianized.

Packaged by stephan beal <stephan@s11n.net> on
${PACKAGE_TIME}.

EOF

}


true && {
    CONTROL=DEBIAN/control
    echo "Generating ${CONTROL}..."
    cat <<EOF > ${CONTROL}
Package: ${DEB_PACKAGE_NAME}
Section: devel
Priority: optional
Maintainer: stephan beal <stephan@s11n.net>
Architecture: ${DEB_ARCH_NAME}
Depends: libc6-dev
Conflicts: libs11n12-dev
Version: ${S11N_DEB_VERSION}
Description: a powerful, flexible serialization framework for C++.
 This package contains all files needed for development, as well as the s11nconvert tool
 and library documentation. Note that an ODD minor version number (e.g. 1.1 or 1.3)
 indicates a beta/development version, not intended for general client-side use,
 whereas EVEN minor numbers (e.g. 1.2 or 1.4) indicate stable versions.
EOF

}


true && {
    dpkg-deb -b ${DEBDIR} ${DEBFILE}
    echo "Package file created:"
    ls -la ${DEBFILE}
    dpkg-deb --info ${DEBFILE}
}


echo "Done :)"
