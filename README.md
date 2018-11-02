# ADUVC

Author: Jakub Wlodek  
Created: July 19, 2018  
Last Updated: November 2, 2018  
Copyright (c): 2018 Brookhaven National Laboratory  

### An EPICS Driver for USB Video Class (UVC) devices

This driver is in development.

### Installation

Prior to installing the ADUVC area detector driver, there are several dependencies that must be met. First, libusb, libjpeg, and cmake and their development packages must be installed. On a debian based linux machine, this can be done with the following command:

```
sudo apt install libusb-dev libjpeg-dev cmake
```
After libusb and the other dependencies have been installed, libuvc, the library for connecting to USB Video Class (UVC) devices must be installed. The easiest way to do this is to enter the adUVCSupport directory, and run the installlibuvc.sh script. The script must be run with sudo access:
```
sudo bash installlibuvc.sh
```
This script will start by cloning the github repository for the libuvc library, and then build it with cmake. The resulting library and include files are then placed in the appropriate locations in the epics build path, as well as in the /usr/local directory, so that libuvc can be accessed in the other support functions. Once libuvc is installed, and epics-base, epics-modules, ADCore, and ADSupport have all been built as well, enter the top ADUVC directory, and simply run:
```
sudo make
```
NOTE: It is important to make the driver and run its IOC with sudo access, because libuvc uses the linux /dev/video0 directory to access the device. If access is attempted without sudo the driver will give a Permission denied error on driver initialization.  

The driver is now installed.  

Libuvc can also be built from source at: https://github.com/ktossell/libuvc.git  
Documentation for the library can be found at: https://int80k.com/libuvc/doc/


### Initial Driver Setup

Once the driver is installed, the UVC camera must be set up. There are some important things to keep in mind when using ADUVC:
* Because libuvc uses /dev/video0 to connect to devices, it requires sole access to the device in addition to root privelages.
* Every UVC camera has either a serial number or a product ID. Either can be used to connect to the camera
* UVC cameras generally don't have many acquisition modes, Usually sticking to 8-bit RGB. This is the only mode the driver supports in its current iteration, though other modes may be added in the future.

To begin the ADUVC setup process, there are two included helper .cpp programs included in the adUVCSupport directory: cameraDetector, and imageCaptureTest. The first of these two, will use libuvc to detect all UVC cameras connected to the system, while the second will use OpenCV highgui with libuvc to test image acquisition. It is recommended to run both programs prior to using the ADUVC driver to make sure that the camera is detected correctly.  

The cameraDetector program lists device information for each of the UVC cameras connected to the system, including their serial number and/or product number if they exist. One of these two values is necessary to connect to the camera. Note that the serial number is actually passed as a const char* to the IOC, so should be placed in quotation marks, while the product number is an integer. More information on running the cameraDetector program can be found in the README file in the appropriate directory.  

Once a serial number or a product number has been discovered using the 


Link: https://int80k.com/libuvc/doc/

Libuvc depends on several packages: libusb-1.0-0 and libusb-1.0-0-dev must both be installed.  

Some detectors that support the UVC standard:  

* https://www.theimagingsource.com/products/industrial-cameras/usb-3.0-color/
* https://www.e-consystems.com/8MP-AF-UVC-USB-Camera.asp
* https://www.framos.com/en/news/videology-introduces-super-speed-uvc-compliant-compact-usb-3.0-board-camera
* https://www.vision-systems.com/articles/2013/04/usb-30-cameras-are-uvc-compliant.html
* https://www.theimagingsource.com/products/industrial-cameras/usb-3.0-monochrome/


### Known issues:

* When building libuvc, the system level jpeg library is used in cmake, but once ADSupport is compiled, a different version is used. This causes an error when converting mjpeg to rgb. THe solution is to either compile libuvc with the jpeg lib in ADSupport, or to set JPEG_EXTERNAL = YES in the CONFIG_SITE.local file in the top level AD configuration directory