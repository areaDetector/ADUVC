all:
	g++ -I../include -L../os/linux-x86_64 uvc_locater.cpp -o uvc_locater ../os/linux-x86_64/libuvc.a -lusb-1.0 -lpthread
clean:
	rm uvc_locater
