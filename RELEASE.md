# ADUVC RELEASES

Author: Jakub Wlodek   

ADUVC requires libusb, libuvc, epics-base, epics-modules, ADCore, and ADSupport. Further installation information can be found in the README file.

<!--RELEASE START-->
Release Notes
=============

R1-2 (11-June-2019)
-----
* Key detector features implemented:
    * Camera modes now read into structs at startup
    * Valid camera mode structs selectable from dropdown in CSS - improves usability
    * Autosave functionality tested and working.
    * More extensive status messages/detector feedback

* Key fixes and improvements
    * Removed unused PVs (ADUVC_VendorID, ADUVC_ProductID)
    * Fixed memory leak caused by early return from frame conversion function on error
    * Added Makefiles to the support modules
    * Documentation updates
    * Added information on using camera with Virtual Box
    * Removed newlines from status messages for better readability.
    * CSS screen updated


R1-1 (28-January-2019)
-----
* Key detector features implemented:
    * Support for Uncompressed and Grayscale images
    * Support for 16-bit images
    * Image timestamps

* Key fixes and improvements
    * Memory copying fixed to remove race condition that could cause plugins to crash
    * Improved conversion between frame format and PV
    * Updated documentation with params[in]/[out]
    * Added LICENSE
    * Code formatting cleaned up (removed all tab characters)
    

R1-0 (7-December-2018)
-----
* Key detector features implemented:
    * UVC Camera features (Sharpness, Backlight, Brightness, Gain, etc.)
    * More format support (MJPEG, RGB, YUYV)
    * Frame size and framerate selection (Previously only 640x480 @ 30 fps)
    * Custom CSS Screen Added

* Key fixes and improvements
    * Fixed error where first frame in stream was corrupt
    * Fixed IOC autosave feature
    * Resolved frame drop due to motion (camera dependant)


R0-1 (Beta) (5-November-2018)
-----
* Key detector features implemented:  
    * Image Acquisition supported and tested.
    * Acquisition mode selection supported and tested
    * Diagnostic information acquisition
    * Plugin interoperability tested
    * Detector IOC written and tested
    * Driver report function implemented

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
    * Limited format support (only mjpeg)
    * Frame size must be specified in the IOC
    * No custom screens (uses ADBase screen)