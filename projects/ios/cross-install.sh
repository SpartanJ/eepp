#!/bin/sh
ln -sf ../../../assets ./eepp.app/
echo "The fist parameter must be the path of the binary file to install. Example: ../../eetest-debug"
cp $1 ./eepp.app/eepp
make install
