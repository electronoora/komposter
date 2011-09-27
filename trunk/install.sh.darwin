#!/bin/bash
#
# linux installer for komposter
#

# don't change the lib path because it has been been compiled into
# the binary
INSTALL_PATH_BIN=/usr/local/bin
INSTALL_PATH_LIB=/usr/local/lib/komposter


echo ::
echo :: This script will install Komposter to binary to ${INSTALL_PATH_BIN} and
echo :: any resources it requires to ${INSTALL_PATH_LIB}. Make sure that you have
echo :: write access \(ie. you are root\) to these directories.
echo ::
echo
echo Press enter to continue or ctrl-C to abort...
read KEYIN

mkdir -p $INSTALL_PATH_BIN
mkdir -p $INSTALL_PATH_LIB
cp komposter $INSTALL_PATH_BIN/
cp -R resources examples doc converter player $INSTALL_PATH_LIB/

echo ::
echo :: Installation complete!
echo ::
echo :: Make sure you have ${INSTALL_PATH_BIN} in your path
echo ::

