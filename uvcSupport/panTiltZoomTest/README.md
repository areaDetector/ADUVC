# Pan/Tilt/Zoom Test

This location includes a test program to check use of Pan/Tilt/Zoom (PTZ) control for UVC cameras.

To begin, use the `cameraDetector` program to find the serial number or product ID of your camera:

```
cd cameraDetector
make
./uvc_locater
```

Next, enter this directory and compile the program with `make`:

```
cd panTiltZoomTest
make
```

Next, run the resulting program supplying a movement direction and the camera identification parameter.
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