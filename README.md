# ADUVC

An EPICS Driver for USB Video Class (UVC) devices

Author: Jakub Wlodek  
Corresponding Author: Kazimierz Gofron  
Created: July 19, 2018  
Last Updated: October 29, 2020  
Copyright (c): 2018-2020 Brookhaven National Laboratory  

Release versions of this driver are available on Github. Release notes are available [here](https://jwlodek.github.io/ADUVC). Please report any problems or feature requests on the issues page [here](https://github.com/areaDetector/ADUVC/issues).

### Installation

**Note - ADUVC has only been tested on linux**

Prior to installing the ADUVC install the required dependencies via your package manager:

```
sudo apt install libusb-dev libusub-1.0-0-dev libjpeg-dev cmake
```

If you wish to use the libjpeg version included with ADSupport, it is important to specifiy that during the build of libuvc - since `cmake` will by default link against the system version, which will cause a run-time conflict with the version with ADSupport. Otherwise, set `JPEG_EXTERNAL=YES` in the `areaDetector/configure/CONFIG_SITE.local` file, and rebuild ADSupport, ADCore, and then ADUVC, which will simply use the system version of the library for everything.

After the other dependencies have been installed, libuvc, the library for connecting to USB Video Class (UVC) devices must be built. The easiest way to do this is to enter the `uvcSupport` directory, and run the provided script, followed by `make`:

```
cd uvcSupport
./install-libuvc.sh
cd ..
make
```

You may need to edit the `EPICS_HOST_ARCH` variable in the `install-libuvc.sh` script prior to running it.

In the event that you do not wish to use the supplied helper script for building libuvc, you may build from [source](https://github.com/libuvc/libuvc.git) yourself.
Documentation for the library can be found [here](https://int80k.com/libuvc/doc/)

### IOC Setup

Start by identifying connected cameras to your system, using the provided `cameraDetector` program in `uvcSupport` (`libuvc` must be installed using the provided script first):

```
cd  uvcSupport/cameraDetector
make
./uvc_locater
```

You will get output similar to the following:

```
UVC initialized successfully
-------------------------------------------------------------
Serial Number:      1275BB10
Vendor ID:          1133
ProductID:          2085
Manufacturer:       (null)
Product:            (null)
UVC Compliance:     0
```

You may need to run the program as `sudo` if camera serial number is locked behind root access.

Next, in the st.cmd file in the `iocs/uvcIOC/iocBoot/iocUVC` directory, locate the ADUVCConfig function call. There are two options for this function, one where the serial number is passed and product ID is set 0, and one where the productID is passed and the serial number is an empty string "". Simply uncomment the way you wish to connect to the device, and place the serial or productID in the appropriate parameter spot. From here the driver IOC is ready to be started with:

```
./st.cmd
```

Again, note that if usb devices are locked behind root privelages you may need to run the IOC with `sudo`, or adjust `udev` permissions/rules.

Further documentation, including CSS screenshots and usage information, is available at the driver's [website](https://jwlodek.github.io/ADUVC).

### Advantages

There are a few advantages the UVC cameras have over traditional industrial cameras:

* **Price** - Cameras can range from 20$ to 500$
* **Ubiquity** - Almost every consumer usb camera supports the UVC protocol, as well as many enterprise cameras
* **PTZ** - Built in Pan/Tilt/Zoom control makes certain devices easy all-in-one monitoring cameras
* **Form Factor** - Huge variety in camera types, including pencil cameras, board cameras, and traditional industrial cameras.

### Example Devices

Below are some devices and specifications that have been deployed using `ADUVC`. If you have used a different camera with `ADUVC`, please feel free to add it to the list below.

Device | Max Resolution | Max Framerate | PTZ | Price ($)
-------|----------------|---------------|--------|--------
[E-Con See3Cam_CU55](https://www.e-consystems.com/5mp-low-noise-usb-camera.asp) | 2248x2048 | 60 | No | 120
[Logitech HD Pro C920](https://www.amazon.com/Logitech-Widescreen-Calling-Recording-Desktop/dp/B006JH8T3S) | 1920x1080 | 60 | No | 129
[Logitech BCC950](https://www.bhphotovideo.com/c/product/877890-REG/Logitech_960_000866_BCC950_ConferenceCam_Video_Conferencing.html) | 1920x1080 | 30 | Yes | 299
[Opti-Tekscope microscope](https://www.amazon.com/gp/product/B0184CCOY0/ref=ppx_yo_dt_b_asin_title_o06_s01?ie=UTF8&psc=1) | 1280x720 | 30 | No | 89
[Fantronics Mobile Snake Cam](https://www.amazon.com/gp/product/B071HYRPND/ref=ppx_yo_dt_b_asin_title_o09_s00?ie=UTF8&psc=1) | 1280x720 | 30 | No | 20
[USBFHD06H-SFV](https://www.amazon.com/gp/product/B07M7JN595/ref=ppx_yo_dt_b_asin_title_o02_s00?ie=UTF8&psc=1) | 1920x1080 | 30 | No | 75
