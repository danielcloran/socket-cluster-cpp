#include <scclient.h>
#include <signal.h>
#include <string.h>

#define ENDPOINT "qa-ws.stuffraiser.com"

volatile bool stop = false;

void logFunction(int level, const char *line)
{
    std::cout << std::string(line);
}

void writerFunction(ScClient *client) {
    json_object *jobj = json_object_new_object();
    json_object *isCodeRunning = json_object_new_int(stop);
    json_object_object_add(jobj, "isCodeRunning", isCodeRunning);

    // The publish function frees the json object after it's sent.
    client->publish("test_channel", jobj);
}
void test(string event, json_object *data) {
    std::cout << "EVENT CALLBACK!" << std::endl;
}

void connected_callback(ScClient *client) {
    client->subscribe("test", &test);

    json_object *jobj = json_object_new_object();
    json_object *isRunningObj = json_object_new_int(1);
    json_object_object_add(jobj, "isRunning", isRunningObj);
    client->publish("test", jobj);

    // client->unsubscribe("test");
}

int main()
{
    // If you want a log output other than the console from LWS.
    lws_set_log_level(7, &logFunction);

    // Allows Ctrl-C to cleanly exit.
    signal(SIGINT, [](int) { stop = true; });

    // Create a new client with Address, Port, and Path.
    ScClient *client = new ScClient(ENDPOINT, 80, "/socketcluster/");

    // Subscribe Callbacks
    client->connected_callback = connected_callback;

    while (!stop && client->socket_connect())
    {
        client->socket_reset();
        usleep(10000);
    }
    client->socket_disconnect();
}