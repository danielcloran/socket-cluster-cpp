#include <SCClient.h>
#include <signal.h>
#include <string.h>
#include <thread>

#define ENDPOINT "127.0.0.1"

class SocketManager
{
public:
    SocketManager()
    {
        theWebsocket = std::make_shared<SCClient>(the_io_context);
    };
    ~SocketManager(){};

    std::thread run()
    {
        return theWebsocket->launch_socket(ENDPOINT, "8000"); // used when local testing
    };

    // These callbacks are not responsible for freeing the memory of the data.
    // The data is freed by the client class.
    void loggedInStatus_callback(std::string event, json_object *data)
    {
        std::cout << "Event: " << event << std::endl;
        std::cout << "Data: " << json_object_to_json_string(data) << std::endl;
    };

    void subscribe_test() {
        // Note the std::bind to callback to the member function
        std::string channel = "user_" + std::to_string(1) + "_loggedInStatus";
        theWebsocket->subscribe(channel, std::bind(&SocketManager::loggedInStatus_callback, this, std::placeholders::_1, std::placeholders::_2));
    }

    void publish_test()
    {
        std::string channel = "user_" + std::to_string(1) + "_loggedInStatus";
        json_object *jobj = json_object_new_object();
        json_object *isLoggedIn = json_object_new_int(true);
        json_object_object_add(jobj, "loggedInStatus", isLoggedIn);
        theWebsocket->publish(channel, jobj);

        // The library also allows user to pass in string directly so the use of json-c is not required.
        theWebsocket->publish(channel, json_object_to_json_string(jobj));

        // SCClient will not free the memory of the json object. You are responsible.
        json_object_put(jobj);
    }

    void stop()
    {
        theWebsocket->stop();
    };

private:
    boost::asio::io_context the_io_context;
    std::shared_ptr<SCClient> theWebsocket;
};


int main()
{
    std::unique_ptr<SocketManager> manager = std::make_unique<SocketManager>();

    // SocketManager->run() returns a thread that should be joined
    // I prefer keeping all .join() calls in the main thread hence returning the thread out.
    std::thread socketThread = manager->run();
    manager->subscribe_test();
    manager->publish_test();

    usleep(1000000);
    /*
    while (true)
    {
        usleep(1000000);
        manager->publish_test();

        // Socket will live in the background as main thread continues (forever if needed)...
        // keep-alive defaults to true the socket will only close if manager->stop(); is called,
        // and will auto-reconnect in case of unexpected disconnection.
    }
    */

    manager->stop();
    socketThread.join();

    return EXIT_SUCCESS;
}