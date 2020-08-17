# UVC Capture test

Author: Jakub Wlodek

The program in this directory can be used to test if a connected UVC device can capture images without setting up the entire driver.

### Installation

This program uses openCV to display the image, so it is required to use the program.

Install opencv using:

```
sudo apt install libopencv-dev
```

You will also need to install libuvc in order to compile this program. For this, edit the
`install-libuvc.sh` script and uncomment the `sudo make install` line. This will install libuvc
to `/usr/local`. Run the script and libuvc should be installed.

Once openCV and libuvc have been installed, compile the program using:

```
make
```

### Usage

First, use the cameraDetector program in the uvcSupport directory to identify either the serial number or the productID of the camera.

Then, run this program as root in one of the following three ways:

```
./capture_test -p $PRODUCT_ID
```

OR

```
./capture_test -s $SERIAL_NUMBER
```

OR

```
./capture_test -h
```

Note that if product id or serial are hidden behind root access, you may need to run as sudo.

A window will open displaying what the camera can see.
The window will stay open for 200 frames of video an will then close automatically.

You may also specify image dimensions. Not all dimensions will be supported, make sure your camera 
supports whatever image size you input. Run the camera detector in verbose mode to see what formats your camera supports, and then run:

```
./capture_test -p $PRODUCT_ID $XSize $Ysize
```

This will stream in the specified size.

**NOTE** This program only streams in 30 fps, and only in the RGB MJPEG format, if you would like to stream at a different framerate you will have to edit the source code.
