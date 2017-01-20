#!/bin/bash
set -e
ROOTDIR=$(readlink -f $0 | xargs dirname)/..
mkdir -p $ROOTDIR/build/x86
rm -rf $ROOTDIR/build/x86/*
if [ $(docker images | grep rallets32 | wc -l) == '0' ]
then
    docker build -t rallets32 -f $ROOTDIR/scripts/Dockerfile.32bit .
fi
docker run --rm --name ralletsbuilder \
           -w /code/rallets \
           -v $ROOTDIR/build/x86:/code \
           -v $ROOTDIR:/code/rallets \
           rallets32 \
           /bin/bash -c "dpkg-buildpackage -uc -us -b && dpkg-buildpackage -Tclean"
