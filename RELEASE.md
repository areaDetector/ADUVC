# ADUVC RELEASES

This driver does not yet have an  official stable release. Currently the 0.1.0 beta release is available, and image acquisition has been tested as working. Currently only 24-bit rgb1 images are supported at 640x480 and 30fps. This will be changed in future releases.   

ADUVC requires libusb, libuvc, epics-base, epics-modules, ADCore, and ADSupport. Further installation information can be found in the README file.

Release Notes
=============

R0-1 (Beta) (2-November-2018)
-----
* Key detector features implemented:  
    * Image Acquisition supported and tested.
    * Acquisition mode selection supported and tested
    * Diagnostic information acquisition
    * Plugin interoperability tested
    * Detector IOC written and tested

* Key Support Features Added
    * Documentation for installation and usage
    * Camera Detector program for detecting UVC cameras and diagnostics
    * Image Acquisition program for testing camera image acquisition
    * libuvc installation script included

* Limitations
    * Only tested on linux-x86_64 systems
    * Framerate goes down during motion (likely due to mjpeg compression)
    * IOC autosave feature not working correctly
    * Many UVC camera functions not yet implemented
    * Limited format and image size support
    * No custom screens (uses ADBase screen)