all: jliptest

jliptest: JLIPTest.cpp
	g++ -Wall -Werror -D_BSD_SOURCE -o jliptest JLIPTest.cpp -ggdb

.DUMMY: clean
clean:
	rm -rf jliptest
