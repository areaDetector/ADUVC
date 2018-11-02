#!bin/bash

# Install libuvc
git clone https://github.com/ktossell/libuvc.git
cd libuvc
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install

mkdir ../../os
mkdir ../../os/linux-x86_64
cp libuvc.so ../../os/linux-x86_64/.
cp libuvc.so.0 ../../os/linux-x86_64/.
cp libuvc.so.0.0.6 ../../os/linux-x86_64/.
cd ../..
rm -rf libuvc
echo "Finished installing libuvc"
