#!/bin/sh
ln -sf ../../../bin/assets ./eepp.app/
echo "The fist parameter must be the path of the binary file to install. Example: ../../bin/eetest-debug.ios"
cp $1 ./eepp.app/eepp
make install $2
