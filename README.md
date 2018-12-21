# ADUVC

Author: Jakub Wlodek  
Corresponding Author: Kazimierz Gofron  
Created: July 19, 2018  
Last Updated: November 2, 2018  
Copyright (c): 2018 Brookhaven National Laboratory  

### An EPICS Driver for USB Video Class (UVC) devices

There is currently one release version of this driver (R1-0). Release notes are available on https://jwlodek.github.io/ADUVC. Please report any problems or feature requests on the issues page on https://github.com/jwlodek/ADUVC.

### Installation

Prior to installing the ADUVC area detector driver, there are several dependencies that must be met. First, libusb, libjpeg, and cmake and their development packages must be installed. On a debian based linux machine, this can be done with the following command:

```
sudo apt install libusb-dev libjpeg-dev cmake
```
If you wish to use the libjpeg version included with ADSupport, it is important to specifiy that during the build of libuvc, because otherwise there will be a conflict when building ADUVC. The simplest solution is to set JPEG_EXTERNAL=YES in the CONFIG_SITE.local file in the configure directory at the top level of areaDetector.  

After libusb and the other dependencies have been installed, libuvc, the library for connecting to USB Video Class (UVC) devices must be installed. The easiest way to do this is to enter the adUVCSupport directory, and run the installlibuvc.sh script. The script must be run with sudo access:
```
bash installlibuvc.sh
```
This script will start by cloning the github repository for the libuvc library, and then build it with cmake. The resulting library and include files are then placed in the appropriate locations in the epics build path, as well as in the /usr/local directory, so that libuvc can be accessed in the other support functions. Once libuvc is installed, and epics-base, epics-modules, ADCore, and ADSupport have all been built as well, enter the top ADUVC directory, and simply run:
```
sudo make
```
**NOTE:** It is important to make the driver and run its IOC with sudo access, because libuvc uses the linux /dev/video0 directory to access the device. If access is attempted without sudo the driver will give a Permission denied error on driver initialization.  

The driver is now installed.  

Libuvc can also be built from source at: https://github.com/ktossell/libuvc.git  
Documentation for the library can be found at: https://int80k.com/libuvc/doc/


### Initial Driver Setup

Once the driver is installed, the UVC camera must be set up. There are some important things to keep in mind when using ADUVC:
* Because libuvc uses /dev/video0 to connect to devices, it requires sole access to the device in addition to root privelages.
* Every UVC camera has either a serial number or a product ID. Either can be used to connect to the camera
* UVC cameras generally don't have many acquisition modes, Usually sticking to 8-bit RGB. This is the only mode the driver supports in its current iteration, though other modes may be added in the future.

To begin the ADUVC setup process, there are two included helper .cpp programs included in the adUVCSupport directory: cameraDetector, and imageCaptureTest. The first of these two, will use libuvc to detect all UVC cameras connected to the system, while the second will use OpenCV highgui with libuvc to test image acquisition. It is recommended to run both programs prior to using the ADUVC driver to make sure that the camera is detected correctly.  

#### Camera Detector

The cameraDetector program lists device information for each of the UVC cameras connected to the system, including their serial number and/or product number if they exist. One of these two values is necessary to connect to the camera. Note that the serial number is actually passed as a const char* to the IOC, so should be placed in quotation marks, while the product number is an integer. More information on running the cameraDetector program can be found in the README file in the appropriate directory.  

#### Capture Test

Once a serial number or a product number has been discovered using the Camera Detector program, you may want to test image acquisition. Installation and run instructions are found in the appropriate README file. Make sure to compile and run as root as required. You should see 200 frames captured by the camera displayed in an OpenCV highgui window. The current default window size is 640x480 at 30 frames per second, though this program may be updated to support user definable parameters.

#### Configuring IOC Startup

To set up the ADUVC ioc, you will need either the device product ID or the serial number. In the st.cmd file in the iocs/adUVCIOC/iocBoot/iocADUVC directory, locate the ADUVCConfig function call. There are two options for this function, one where the serial number is passed and product ID is set 0, and one where the productID is passed and the serial number is an empty string "". Simply uncomment the way you wish to connect to the device, and place the serial or productID in the appropriate parameter spot. From here the driver IOC is ready to be started with:
```
sudo ./startEPICS.sh
```

Further documentation is available at https://jwlodek.github.io/ADUVC  

### Possible use cases

Most UVC cameras are cheap and easy to set up, making them perfect for a variety of use cases including:

* Plugin testing and development -> easy to set up, making them great test benches.
* Monitoring -> Cheap and small, so can be used as a monitoring device, especially paired up with small devices like Raspberry Pis, or Arduinos.
* Use in small confined areas -> UVC cameras are available in many configurations including tiny pencil cameras that can be used to view confined areas.

There are also several more traditional industrial cameras that use the UVC standard:

* https://www.theimagingsource.com/products/industrial-cameras/usb-3.0-color/
* https://www.e-consystems.com/8MP-AF-UVC-USB-Camera.asp
* https://www.framos.com/en/news/videology-introduces-super-speed-uvc-compliant-compact-usb-3.0-board-camera
* https://www.vision-systems.com/articles/2013/04/usb-30-cameras-are-uvc-compliant.html
* https://www.theimagingsource.com/products/industrial-cameras/usb-3.0-monochrome/


### Some Known Issues

* When building libuvc, the system level jpeg library is used in cmake, but once ADSupport is compiled, a different version is used. This causes an error when converting mjpeg to rgb. THe solution is to either compile libuvc with the jpeg lib in ADSupport, or to set JPEG_EXTERNAL = YES in the CONFIG_SITE.local file in the top level AD configuration directory
* Certain cameras only support one framerate per frame size, so setting the framerate PV may not affect the actual image rate
* Not all cameras support RGB raw images, and so YUYV may be the only supported raw image format
* In cheaper cameras framerate drops when there is lots of motion. This is due to image processing on the camera itself, not due to the driver.
* First frame in mjpeg stream can be corrupted, causing a UVC Error, however, the driver continues and each subsequent frame is uncorrupted.

