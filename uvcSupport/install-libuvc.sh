#!/bin/bash

EPICS_HOST_ARCH=linux-x86_64

# Install libuvc by cloning from github and running cmake
git clone https://github.com/jwlodek/libuvc.git
cd libuvc
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# This line will compile the program and place a copy of the libs in /usr/local. 
# It is recommended to keep it uncommented to compile the helper programs.
sudo make install

# Moves libs into the correct positions for EPICS.
# Note: If included 'include' files are out of date, make sure to copy those from the build folder as well
mkdir ../../os
mkdir ../../os/$EPICS_HOST_ARCH
cp libuvc.so ../../os/$EPICS_HOST_ARCH/.
cp libuvc.so.0 ../../os/$EPICS_HOST_ARCH/.
cp libuvc.so.0.0.6 ../../os/$EPICS_HOST_ARCH/.
cd ../..
rm -rf libuvc
echo "Finished installing libuvc"
