all: dvdemu

dvdemu: DVDEmu.cpp
	g++ -Wall -Werror -D_BSD_SOURCE -o dvdemu DVDEmu.cpp Serial.cpp Video.cpp Toshiba_SD-B100.cpp Victor_XV-D701.cpp -ggdb

.DUMMY: clean
clean:
	rm -rf dvdemu

