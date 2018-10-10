#!bin/bash

# Install the necessary packages
sudo apt-get install libusb-1.0
sudo apt-get install cmake
sudo apt-get install libjpeg-dev

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
