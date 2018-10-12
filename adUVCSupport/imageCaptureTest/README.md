# UVC Capture test

The program in this directory can be used to test if a connected UVC device can capture images without setting up the entire driver.

### Installation

This program uses openCV to display the image, so it is required to use the program.

Install opencv using:
```
sudo apt install libopencv-dev
```

Once openCV and libuvc have been installed, compile the program as root using
```
g++ capture_test.cpp -o capture_test -luvc $(pkg-config --cflags --libs opencv)
```

### Usage

First, use the cameraDetector program in the adUVCSupport directory to identify either the serial number or the productID of the camera.

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

A window will open displaying what the camera can see.
The window will stay open for 200 frames of video an will then close automatically.