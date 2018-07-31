# Makefile

all: video camera

camera: shutter920.cpp
	g++ shutter920.cpp -o shutter920 -I/usr/local/include/opencv2 -I/usr/local/include/opencv -L/usr/local/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_videoio -lpthread

video: video920.cpp
	g++ video920.cpp -o video920 -I/usr/local/include/opencv2 -I/usr/local/include/opencv -L/usr/local/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_videoio -lpthread

clean:
	rm -f shutter920 video920

