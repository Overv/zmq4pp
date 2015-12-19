example: example.cc zmq4.hpp
	g++ -std=c++11 -o example example.cc -L/usr/local/lib -lzmq

.PHONY: clean
clean:
	rm -f example
