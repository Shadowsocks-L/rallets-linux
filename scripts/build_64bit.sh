#!/bin/bash
set -e
ROOTDIR=$(readlink -f $0 | xargs dirname)/..
mkdir -p $ROOTDIR/build/x64
rm -rf $ROOTDIR/build/x64/*
cd $ROOTDIR
dpkg-buildpackage -uc -us -b
mv $ROOTDIR/../rallets*.deb $ROOTDIR/build/x64
mv $ROOTDIR/../rallets*.changes $ROOTDIR/build/x64
dpkg-buildpackage -Tclean
