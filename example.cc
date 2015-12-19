#include "zmq4.hpp"
#include <iostream>

int main() {
    zmq::context context;

    zmq::socket socket(context, zmq::socket_type::sub);
    socket.connect("tcp://123.456.789.000:1234");
    socket.subscribe("");

    auto parts = socket.recv_multipart();

    std::cout << "topic: " << parts[0] << std::endl;
    std::cout << "data: " << parts[1] << std::endl;

    return 0;
}
