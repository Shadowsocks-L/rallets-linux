#!/usr/bin/bash

qmake -qt=5 INSTALL_PREFIX=./build
make
make install
