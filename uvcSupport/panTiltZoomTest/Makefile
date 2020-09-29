all:
	g++ -I../include -L../os/linux-x86_64 ptz.cpp -o ptz_test ../os/linux-x86_64/libuvc.a -lusb-1.0 -lpthread
clean:
	rm ptz_test
