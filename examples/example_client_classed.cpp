#include <scclient.h>
#include <signal.h>
#include <string.h>
#include <thread>

#define ENDPOINT "127.0.0.1"

class SocketManager
{
public:
    SocketManager()
    {
        client = new ScClient(ENDPOINT, 80, "/socketcluster/");
        client->connected_callback = std::bind(&SocketManager::connect_cb, this, std::placeholders::_1);
    };
    ~SocketManager() { delete client; };

    void connect_cb(ScClient *client) { std::cout << "Connected" << std::endl; };
    std::thread run()
    {
        std::thread socketThread([this]()
                                 {
            while (!stop_flag && !client->socket_connect())
            {
                client->socket_reset();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } });

        while (!client->connection_flag)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return socketThread;
    };

    void test_cb(string event, json_object *data)
    {
        std::cout << "Event: " << event << std::endl;
        std::cout << "Data: " << json_object_to_json_string(data) << std::endl;
        // Can use client in here freely as this is a member function.
        client->unsubscribe("test");
    };

    void publish_test()
    {
        // Note the std::bind to callback to the member function
        client->subscribe("test", std::bind(&SocketManager::test_cb, this, std::placeholders::_1, std::placeholders::_2));

        // Lookup json-c for format surrounding the passed in json_object
        // Possible future TODO: allow user to pass in string directly and parse json_object from that
        json_object *jobj = json_object_new_object();
        json_object *isCodeRunning = json_object_new_int(stop_flag);
        json_object_object_add(jobj, "isCodeRunning", isCodeRunning);

        /*
            The publish function frees the json object after it's sent
            so don't worry about allocating and then leaving scope.
        */
        client->publish("test", jobj);
    }

    void stop()
    {
        stop_flag = true;
        client->socket_disconnect();
    };

private:
    ScClient *client;
    volatile bool stop_flag = false;
};

int main()
{
    SocketManager *manager = new SocketManager();
    // SocketManager->run() returns a thread that should be joined
    std::thread socketThread = manager->run();
    manager->publish_test();

    // Wait for 3 seconds to process events
    usleep(3000000);

    manager->stop();
    socketThread.join();

    delete manager;
    return 0;
}