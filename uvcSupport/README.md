# Installing libuvc:

#### Dependencies:

Libuvc depends on libusb.1.0, which can be installed from your linux distribution, or from 
source.

```
sudo apt install libusb.1.0
sudo apt install libusb-dev
```

#### Installation:

Provided is a script that can be used for easily installing libuvc on a linux. 

```
./install-libuvc.sh
```

It clones the github repository containing the library, runs cmake, and installs the include files and built libraries into the 
appropriate folders. Once this script has been run, simply compile the driver from the top level
ADUVC directory.

Optionally, you may also edit the script to uncomment the `sudo make install` command, which will install
libuvc to a system location.

#### Additional tools:

This directory contains two additional simple tools for helping with setting up your ADUVC IOC.

For both, you will need to install libuvc with the script as described above. See the README files
in the appropriate directories for more details.

The first utility is a camera detector program, which is used to find supported operating modes
and connection attributes for cameras. The second is a simple program that tests image acquisition using
libuvc and OpenCV for a specified camera.

Before creating an IOC for the device, use the camera detector program to find the following 
information that will be required for the configuration later on.

These include:

* Serial Number
* Framerate
* Resolution (X and Y)
* Video Format

UVC cameras have set formats that they support, and as a result, must be configured before use.
