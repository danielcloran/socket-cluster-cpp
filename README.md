# socket-cluster-cpp
This socketcluster client is developed using [boost beast](https://github.com/boostorg/beast) and [json-c](https://github.com/json-c/json-c) library written in C++ and is compiled using [CMake](http://www.cmake.org). So far, this library only supports very simple threadsafe publish, subscribe, keep-alive, and reconnect. Pending interest... more functionality can be added.

See [examples](https://github.com/danielcloran/socket-cluster-cpp/tree/main/examples) for implementation.

### Overview
The library is structured with a threadsafe message queue and a subscription queue... the message queue is used to send messages to the server and the subscription queue is used to receive messages from the server and store callbacks. The publish and subscribe functions are implemented using the same pattern shown by [socket-cluster](https://github.com/SocketCluster/socketcluster).

### Installation
The two required components of this library are boost beast and json-c. The boost beast library is used to implement the websocket and the json-c library is used to parse and serialize json messages. 

- The boost beast library (VERSION 1.76.0 required for the websocket) is available on [boost.org](https://www.boost.org/). 
- Json-c is one of the most performant and easy-to-use JSON implementations in c. To use json-c objects in client follow [tutorials on json-c](https://linuxprograms.wordpress.com/2010/05/20/json-c-libjson-tutorial/).

Compile the examples using:
 
- mkdir build
- cd build
- cmake ..
- make
- ./example_client_simple or ./example_client_classed

### Testing
Sections of this library have been used in a production environment (8 hours a day, 6 days a week). The maximum output and input rates have not been tested. **Both example programs have been profiled and show no memory leaks.** 