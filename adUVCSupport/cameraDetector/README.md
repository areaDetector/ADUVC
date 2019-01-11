# UVC Device locater

Author: Jakub Wlodek

The .cpp file in this directory can be used to locate UVC devices and to get
critical information about them.

#### Compilation:

To compile the helper program, switch to a root account, and compile using:

```
sudo su
g++ uvc_locater.cpp -o uvc_locater -luvc
```

**NOTE:** make sure that libuvc is in your library path. if you used the script in
adUVCSupport to install it, it should have been placed there automatically.


#### Usage:

To use the helper, run it as root. These are the possible commands:

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

will list all of the uvc devices connected to the machine, and some basic properties.
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

Some pointers for finding supported data types:

* Run the locater in detailed mode on the specific camera. Supported frame formats will be listed as Uncompressed or MJPEG
* For Uncompressed formats, look at the 'GUID' field, and this will tell you which format is used for uncompressed. (Y16 -> Gray16, YUY2 -> YUYV, etc.)