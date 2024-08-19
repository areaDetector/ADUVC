# ADUVC Utilities

This directory contains several simple utility programs for use with ADUVC. To compile them, first compile `libuvc` by running `make` in the `uvcSupport` directory, and then run:

```
make clean all
```

to build all programs, or 

```
make clean listcameras
```

to build the basic UVC camera device locater program. Note that for the capture test program, opencv will need to be installed on your system.

#### Compilation:

```
make listcameras
```

#### Usage:

To use the locater, you can run it with these possible commands:

```
./uvc_locater -h 
```

OR

```
./uvc_locater --help
```

will list some help information about the helper program.


```
./uvc_locater
```

will list all of the uvc devices connected to the machine, and some basic properties. Certain properties
are locked behind root access, and so will require running the program as root.

From here, you can get a UVC camera's serial number, which can be used in the following way:

```
./uvc_locater -s $SERIAL_NUMBER
```

OR

```
./uvc_locater --serial $SERIAL_NUMBER
```

This will print out detailed information about the camera, including supported video
formats, framerates, and resolutions, all of which must be used in the st.cmd ioc
startup script.

```
./uvc_locater -p $PRODUCT_ID
```

OR

```
./uvc_locater --product $PRODUCT_ID
```

This will print the same information as the search by serial, but will search by product ID.

### Pan/Tilt/Zoom Test

#### Compilation

```
make clean ptztest
```

#### Usage

Next, run the resulting program supplying a movement direction and the camera identification parameter.
For finding the serial/product number of your camera, use `uvc_locater`, as described above.
For example, for a camera with a serial number B56C90EF that we wish to move left, run:

```
./pt_test left -s B56C90EF
```

OR

```
./pt_test -s B56C90EF left
```

Note that you can give either the direction or the connection parameter first. For more help, run with the `-h` flag:


```
./pt_test -h
```


### UVC Capture test

#### Compilation

First, make sure that `opencv` is installed on your system and in the include/library search path.

Then, compile with:

```
make clean captest
```

#### Usage

First, find your camera's serial/product number with `uvc_locater`

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

**NOTE** This program only streams in 30 fps, and only in the RGB MJPEG format. If your camera does not support such a mode the capture test will not function.


### Finding supported data types

**UPDATE - As of R1-8, the driver automatically reads supported formats (up to 16). The detailed description may still be useful for setting up non-root camera access**

Some pointers for finding supported data types:

* Run the `uvc_locater` in verbose mode on the specific camera. Supported frame formats will be listed as Uncompressed or MJPEG
* For Uncompressed formats, look at the 'GUID' field, and this will tell you which format is used for uncompressed. (Y16 -> Gray16, YUY2 -> YUYV, etc.)
