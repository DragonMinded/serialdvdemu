all: dvdemu

dvdemu: DVDEmu.cpp DVDEmu.h Serial.cpp Serial.h Video.cpp Video.h Toshiba_SD-B100.cpp Toshiba_SD-B100.h Victor_XV-D701.cpp Victor_XV-D701.h
	g++ -Wall -Werror -D_BSD_SOURCE -pthread -lcurses -o dvdemu DVDEmu.cpp Serial.cpp Video.cpp Toshiba_SD-B100.cpp Victor_XV-D701.cpp -ggdb

.DUMMY: clean
clean:
	rm -rf dvdemu

