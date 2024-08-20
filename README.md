# ADUVC

An [EPICS](www.aps.anl.gov/epics) [areaDetector](https://github.com/areaDetector/areaDetector/blob/master/README.md) driver for communicating with USB Video Class (UVC) devices, including consumer webcams and some industrial cameras, using the open source [libuvc library](https://github.com/libuvc/libuvc).

The driver has been tested on 64 bit linux only, but it is possible that it will also work on windows and other architectures as well. 

It was tested with many devices, including the following:

Device | Max Resolution | Max Framerate | PTZ | Price ($)
-------|----------------|---------------|--------|--------
[E-Con See3Cam_CU55](https://www.e-consystems.com/5mp-low-noise-usb-camera.asp) | 2248x2048 | 60 | No | 120
[Logitech HD Pro C920](https://www.amazon.com/Logitech-Widescreen-Calling-Recording-Desktop/dp/B006JH8T3S) | 1920x1080 | 60 | No | 129
[Logitech BCC950](https://www.bhphotovideo.com/c/product/877890-REG/Logitech_960_000866_BCC950_ConferenceCam_Video_Conferencing.html) | 1920x1080 | 30 | Yes | 299
[Opti-Tekscope microscope](https://www.amazon.com/gp/product/B0184CCOY0/ref=ppx_yo_dt_b_asin_title_o06_s01?ie=UTF8&psc=1) | 1280x720 | 30 | No | 89
[Fantronics Mobile Snake Cam](https://www.amazon.com/gp/product/B071HYRPND/ref=ppx_yo_dt_b_asin_title_o09_s00?ie=UTF8&psc=1) | 1280x720 | 30 | No | 20
[USBFHD06H-SFV](https://www.amazon.com/gp/product/B07M7JN595/ref=ppx_yo_dt_b_asin_title_o02_s00?ie=UTF8&psc=1) | 1920x1080 | 30 | No | 75

Additional information:

* [Documentation](https://github.com/areaDetector/ADUVC/blob/master/docs/ADUVC/ADUVC.rst)
* [libuvc API docs](https://libuvc.github.io/libuvc)
* [Release notes](https://github.com/areaDetector/ADUVC/blob/master/RELEASE.md)

Author: Jakub Wlodek
