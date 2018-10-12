# ADUVC

### Author: Jakub Wlodek
### Created: July 19, 2018
### Copyright (c): 2018 Brookhaven National Laboratory

### An EPICS Driver for USB Video Class (UVC) devices

This driver is in development.

Some required libraries/packages for this driver will be libuvc which can be downloaded and built from github:  
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