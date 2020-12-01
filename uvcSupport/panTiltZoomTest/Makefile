all:
	g++ -I../include -L../../lib/linux-x86_64 pt.cpp -o pt_test ../../lib/linux-x86_64/libuvc.a -lusb-1.0 -lpthread
	g++ -I../include -L../../lib/linux-x86_64 zoom.cpp -o zoom_test ../../lib/linux-x86_64/libuvc.a -lusb-1.0 -lpthread
clean:
	rm pt_test
	rm zoom_test
