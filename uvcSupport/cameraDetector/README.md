# UVC Device locater

Author: Jakub Wlodek

The .cpp file in this directory can be used to locate UVC devices and to get
critical information about them.

#### Compilation:

To compile this helper program, you must first compile libuvc. The easiest way to do this is to use the
installation script in the `uvcSupport` directory:

```
./install-libuvc.sh
```

This should use CMake to compile the library, and copy the necessary build artefacts into the correct
locations for compilation.

Once this completes successfully, you can compile the program by just running:

```
make
```

**NOTE:** This will link against pthread, libusb-1.0. You must have these installed to
compile libuvc, along with these helper programs.

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

#### Finding supported data types:

**UPDATE - As of R1-3, the driver automatically reads supported formats (up to 7). The detailed description may still be useful for setting up non-root camera access**

Some pointers for finding supported data types:

* Run the locater in detailed mode on the specific camera. Supported frame formats will be listed as Uncompressed or MJPEG
* For Uncompressed formats, look at the 'GUID' field, and this will tell you which format is used for uncompressed. (Y16 -> Gray16, YUY2 -> YUYV, etc.)
