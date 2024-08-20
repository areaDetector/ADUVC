# ADUVC Support

This directory contains modified sources for [`libuvc`](https://github.com/libuvc/libuvc) for use with `ADUVC`. They have been modifed to build via the EPICS build system. Also included are some helper programs for testing `libuvc`, and for detecting connected camera information.

### Installing libuvc

The `libuvc` library  depends on libusb.1.0, which can be installed from your linux distribution, or from source. It also requires libjpeg, which will be pulled from `ADSupport` by default.


As of release `R1-6`, you can build `libuvc` by simply running `make` in this directory. It will install the library binary files to `../lib/EPICS_HOST_ARCH`. You can also install outside the `EPICS` build system, using either the `install-libuvc.sh` script, or manually.

### Additional tools

This directory contains several additional simple tools for helping with setting up your ADUVC IOC.

For these, you will first need to install libuvc to the `../lib/EPICS_HOST_ARCH` location as described above.

Then, simply enter the `utils` directory, and compile the tool of your choice - see the `README` in that directory for more details.

