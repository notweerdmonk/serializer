
all: test

test: serializer.h test.cpp
	g++ -g -std=c++11 -I . -o test test.cpp

clean:
	rm test data
