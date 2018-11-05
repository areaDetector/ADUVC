#!bin/bash

# Install libuvc by cloning from github and running cmake
git clone https://github.com/ktossell/libuvc.git
cd libuvc
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
# This line will compile the program and place a copy of the libs in /usr/local
sudo make install

# Moves libs into the correct positions for EPICS.
# Note: If included 'include' files are out of date, make sure to copy those from the build folder as well
mkdir ../../os
mkdir ../../os/linux-x86_64
cp libuvc.so ../../os/linux-x86_64/.
cp libuvc.so.0 ../../os/linux-x86_64/.
cp libuvc.so.0.0.6 ../../os/linux-x86_64/.
cd ../..
rm -rf libuvc
echo "Finished installing libuvc"
