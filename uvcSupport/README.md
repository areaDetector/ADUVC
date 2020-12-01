# ADUVC Support

This directory contains modified sources for [`libuvc`](https://github.com/libuvc/libuvc) for use with `ADUVC`. They have been modifed to build via the EPICS build system. Also included are some helper programs for testing `libuvc`, and for detecting connected camera information.

#### Installing libuvc

The `libuvc` library  depends on libusb.1.0, which can be installed from your linux distribution, or from source. It also requires libjpeg, which will be pulled from `ADSupport` by default.

```
sudo apt install libusb.1.0
sudo apt install libusb-dev
```

As of release `R1-6`, you can build `libuvc` by simply running `make` in this directory. It will install the library binary files to `../lib/EPICS_HOST_ARCH`. You can also install outside the `EPICS` build system, using either the `install-libuvc.sh` script, or manually.

#### Additional tools

This directory contains three additional simple tools for helping with setting up your ADUVC IOC.

For these, you will need to install libuvc to the `../lib/EPICS_HOST_ARCH` location as described above. See the README files in the appropriate directories for more details.

The first utility is a camera detector program, which is used to find supported operating modes and connection attributes for cameras. The second is a simple program that tests image acquisition using libuvc and OpenCV for a specified camera. The third is a program for testing PTZ support on a connected camera.

Before creating an IOC for the device, use the camera detector program to find the following information that will be required for the configuration later on.

These include:

* Serial Number
* Framerate
* Resolution (X and Y)
* Video Format

UVC cameras have set formats that they support, and as a result, must be configured before use.
