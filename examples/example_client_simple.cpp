#include <SCClient.h>
#include <signal.h>
#include <string.h>

#define ENDPOINT "127.0.0.1"

// These callbacks are not responsible for freeing the memory of the data.
// The data is freed by the client class.
void test(std::string event, json_object *data)
{
    std::cout << "In the event callback, data: " << json_object_to_json_string(data) << std::endl;
}

void test_publish(std::shared_ptr<SCClient> theWebsocket)
{
    json_object *data = json_object_new_object();json_object *jobj = json_object_new_object();
    json_object *isCodeRunning = json_object_new_int(true);
    json_object_object_add(jobj, "isCodeRunning", isCodeRunning);
    theWebsocket->publish("test", json_object_to_json_string(jobj));
    json_object_put(jobj);
}

int main()
{
    boost::asio::io_context the_io_context;
    std::shared_ptr<SCClient> theWebsocket = std::make_shared<SCClient>(the_io_context);
    std::thread socketThread = theWebsocket->launch_socket(ENDPOINT, "8000");

    // Go ahead and continue running and doing stuff in the main thread after launching socket.

    /*
        Subscribe Callbacks
            Note: these can also be member functions using:
            client->connected_callback = std::bind(&myClass::myFunc, myInstance, std::placeholders::_1);
            See example_client_classed.cpp for an example of this.
    */
    theWebsocket->subscribe("test", test);

    usleep(1000000);
    test_publish(theWebsocket);
    usleep(1000000);

    // Unsubscribe from the event (optional)
    theWebsocket->unsubscribe("test");

    // Close the socket
    theWebsocket->stop();
    socketThread.join();

    return EXIT_SUCCESS;
}