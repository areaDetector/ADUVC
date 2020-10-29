
# ADUVC Known Issues

Below is a list of known issues/fixes found with using `ADUVC` with usb cameras.

* When building libuvc, the system level jpeg library is used in cmake, but once ADSupport is compiled, a different version is used. This causes an error when converting mjpeg to rgb. The solution is to either compile `libuvc` with the jpeg lib in ADSupport, or to set `JPEG_EXTERNAL = YES` in the `areaDetector/configure/CONFIG_SITE.local` file.
* Certain cameras only support one framerate per frame size, so setting the framerate PV may not affect the actual image rate.
* Most cameras have a limited selection of fixed acquisition modes (certain framerates with certain sizes). Use the cameraDetector helper program to identify these modes.
* In cheaper cameras framerate drops when there is lots of motion. This is due to image processing on the camera itself, not due to the driver.
* First frame in mjpeg stream can be corrupted, causing a UVC Error, however, the driver continues and each subsequent frame is uncorrupted.
* If using ADUVC with Virtualbox, you need to passthrough the hold of the camera to the guest OS. Instructions for doing so  can be found [here](https://scribles.net/using-webcam-in-virtualbox-guest-os-on-windows-host/).


### Fixing issues with root ownership of UVC devices

* The  USB camera device is typically owned by root, which prevents EPICS IOC from running as a non-root user, and automatic startup using procServer. To grant access to USB camera device by  other users, such as softioc, we wrote udev rules.

```
kgofron@xf17bm-ioc2:/etc/udev/rules.d$ more usb-cams.rules
# cam1 f007
SUBSYSTEM=="usb", ATTRS{idVendor}=="f007", OWNER="softioc", GROUP="softioc", MODE="0666", SYMLINK="cam1"
# cam2 0c45
SUBSYSTEM=="usb", ATTRS{idVendor}=="0c45", OWNER="softioc", GROUP="softioc", MODE="0666", SYMLINK="cam2"
# cam3 2560
SUBSYSTEM=="usb", ATTRS{idVendor}=="2560", OWNER="softioc", GROUP="softioc", MODE="0666", SYMLINK="cam3"
# cam4 eb1a (not attached)
SUBSYSTEM=="usb", ATTRS{idVendor}=="eb1a", OWNER="softioc", GROUP="softioc", MODE="0666", SYMLINK="cam4"
# cam5 eb1a
SUBSYSTEM=="usb", ATTRS{idVendor}=="eb1a", OWNER="softioc", GROUP="softioc", MODE="0660", SYMLINK="cam5"

```

* Some inexpensive UVC cameras do not have Serial Numbers written to the device by vendors. In the event that this is the case, the Product ID was available, however, this comes with the limitation that only one camera per product ID can be running in as an IOC at a time. It is possible that this could be circumvented with some expanded udev rules, as below.

```
# maybe something like this to get the identical vendor/product/serial# cams working simultaneously
#SUBSYSTEM=="usb", ATTRS{idVendor}=="eb1a", KERNELS=="1-8:1.0", OWNER="softioc", GROUP="softioc", MODE="0660", SYMLINK="cam5"