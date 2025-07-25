# ADUVC RELEASES

Author: Jakub Wlodek

ADUVC requires libusb, libuvc, epics-base, epics-modules, ADCore, and ADSupport. Further installation information can be found in the README file.

<!--RELEASE START-->
Release Notes
=============

R1-8 (20-Aug-2024)
----
* Features Added
    * Increase max number of auto-configured operating modes to 16
    * Improved organization of utility programs

* Fixes
    * Resolve bug with size PVs being incorrect for color images
    * Cleanup docs


R1-7 (27-January-2023)
----
* Features Added
    * New option to connect to device by device index - needed for connecting to identical devices with no serial number
    * Add printing of HEX value of product and vendor IDs to `uvc_locater` - useful for generating udev rules
    * New flag for `uvc_locater` to print more easily machine readable output
    * New simple utility script for generating udev rule files from output of `uvc_locater`

* Fixes
    * Minor bug with continuous acquisition mode fixed
    * Avoid "Auto Adjust" as default behavior


R1-6 (1-December-2020)
----
* Features Added
    * `libuvc` now builds via the EPICS build system


R1-5 (29-October-2020)
----
* Features Added
    * Pan/Tilt/Zoom control (PTZ)
    * Improved static build control
    * Improved helper tools
    * Improved script for building libuvc

* Fixes/Improvements
    * Minor code changes to remove tabs and replace with spaces

R1-4 (18-August-2020)
----
* Features Added
    * Added ReST based documentation for new areaDetector doc format
    * Static build support (for libuvc)

* Fixes/Improvement
    * Fix segmentation fault caused by disconnect
    * Libuvc install script fixed to support include files from build
    * Minor fixes to build system to allow for static building
    * Folder + file name changes to better follow AD naming standard
    * Many code readability improvements
    * Allow for helper utilities to compile with non-system installed libuvc

* Known Issues
    * `libuvc` relies on `libusb-1.0`, not `libusb`.
    * When camera is not connected, driver segmentation faults - should be handled more gracefully.


R1-3 (06-September-2019)
-----
* Key detector features implemented:
    * Added IOC feature to auto-adjust camera settings based on operating mode.

* Key fixes and improvements
    * Minor screen updates
    * Repoint libuvc upstream repo. New repo has issues with CMake file
    * Add documentation for fixing root ownership issues of UVC devices

* Known Issues
    * Auto adjust feature can break with certain odd configurations of the camera
    * Uses array size to determine 8/16 bit and Mono/RGB, but certain compressed formats break the adjuster.

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
