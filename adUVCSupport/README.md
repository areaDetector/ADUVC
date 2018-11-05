# Installing libuvc:

#### Dependencies:

Libuvc depends on libusb.1.*, which can be installed from your linux distribution, or from 
source.
```
sudo apt install libusb.1.0
sudo apt install libusb-dev
```

#### Installation:

Provided is a script that can be used for easily installing libuvc on a debain based machine. 
```
bash installlibuvc.sh
```

It clones the github repository containing the library, runs cmake, and installs the libs into the 
appropriate folder. Once this script has been run, simply compile the driver from the top level
ADUVC directory.

#### Additional tools:

Also included in this directory is a folder titled cameraDetector.
Within this directory is a helper .cpp file that contains code for discovering UVC devices connected
to the specific machine.

The README.md file in said directory contains information on compiling and running the tool.
Make sure to first install libuvc and then run the tool, prior to booting the IOC,
as there are several values that must be passed to the IOC in the configuration.  
  
These include:

* Serial Number (really a const char*)
* Framerate
* Resolution (X and Y)
* Video Format

UVC cameras have set formats that they support, and as a result, must be configured before use.
