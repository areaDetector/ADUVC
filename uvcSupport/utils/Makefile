all: listcameras ptztest usbreset

listcameras:
	mkdir -p bin
	gcc -I../../include -L../../lib/linux-x86_64 uvc_locater.c -o bin/uvc_locater ../../lib/linux-x86_64/libuvc.a -lusb-1.0 -lpthread

ptztest: listcameras
	mkdir -p bin
	gcc -I../../include -L../../lib/linux-x86_64 pan_tilt_test.c -o bin/pt_test ../../lib/linux-x86_64/libuvc.a -lusb-1.0 -lpthread
	gcc -I../../include -L../../lib/linux-x86_64 zoom_test.c -o bin/zoom_test ../../lib/linux-x86_64/libuvc.a -lusb-1.0 -lpthread

usbreset:
	mkdir -p bin
	gcc -o bin/software_usb_hotplug usb_reset.c

captest: listcameras
	mkdir -p bin
	g++ -I../../include -L../../lib/linux-x86_64 capture_test.cpp -o bin/capture_test -luvc -lopencv_core -lopencv_imgproc -lopencv_highgui

clean:
	rm -r bin
