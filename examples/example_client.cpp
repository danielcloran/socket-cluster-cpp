#include <scclient.h>
#include <signal.h>
#include <string.h>

#define ENDPOINT "qa-ws.stuffraiser.com"

volatile bool stop = false;

void logFunction(int level, const char *line)
{
    std::cout << std::string(line);
}

void test(string event, json_object *data) {
    std::cout << "In the event callback, data: " << json_object_to_json_string(data) << std::endl;
}

void example_publish(ScClient *client) {
    // Lookup json-c for format surrounding the passed in json_object
    // Possible future TODO: allow user to pass in string directly and parse json_object from that
    json_object *jobj = json_object_new_object();
    json_object *isCodeRunning = json_object_new_int(stop);
    json_object_object_add(jobj, "isCodeRunning", isCodeRunning);

    /*
        The publish function frees the json object after it's sent
        so don't worry about allocating and then leaving scope.
    */
    client->publish("test", jobj);
}

void connected_callback(ScClient *client) {
    std::cout << "Connected!" << std::endl;
    client->subscribe("test", &test);
    example_publish(client);
}

void connected_error_callback(std::string reason) {
    std::cout << "Disconnected: " << reason << std::endl;
}

void disconnected_callback(std::string reason) {
    std::cout << "Disconnected: " << reason << std::endl;
}

int main() {
    // If you want a log output other than the console from LWS.
    lws_set_log_level(7, &logFunction);

    // Allows Ctrl-C to cleanly exit.
    signal(SIGINT, [](int) { stop = true; });

    // Create a new client with Address, Port, and Path.
    ScClient *client = new ScClient(ENDPOINT, 80, "/socketcluster/");

    /*
        Subscribe Callbacks
            Note: these can also be member functions using:
            client->connected_callback = std::bind(&myClass::myFunc, myInstance, std::placeholders::_1);
    */
    client->connected_callback = connected_callback;
    client->connected_error_callback = connected_error_callback;
    client->disconnected_callback = disconnected_callback;


    /*
        Since the client is threadsafe, we could move this while to a thread
        and have it run in the background. Subscriptions can happen from anywhere...
        remember any values edited in the callback will need to be threadsafe.
    */
    while (!stop && client->socket_connect())
    {
        client->socket_reset();
        usleep(10000);
    }
    client->socket_disconnect();
}