# socket-cluster-cpp
This socketcluster client is developed using [boost](https://github.com/warmcat/libwebsockets) and [json-c](https://github.com/json-c/json-c) library in C++. So far, this library only supports very simple publish and subscribe, pending interest... more functionality can be added.

See [examples](https://github.com/danielcloran/socket-cluster-cpp/tree/main/examples) for implementation.

Libwebsockets is a lightweight pure C library built to use minimal CPU and memory resources, and provide fast throughput.It is a CMake based project that has been used in a variety of OS contexts including Linux (uclibc and glibc), ARM-based embedded boards, MBED3, MIPS / OpenWRT, Windows, Android, Apple iOS and even Tivo. It's used all over the place including The New York Times customer-facing servers and BMW. Architectural features like nonblockinng event loop, zero-copy for payload data and FSM-based protocol parsers make it ideal for realtime operation on resource-constrained devices.

Json-c is one of the most performant and easy-to-use JSON implementations in c. To use json-c objects in client follow [tutorials on json-c](https://linuxprograms.wordpress.com/2010/05/20/json-c-libjson-tutorial/).
