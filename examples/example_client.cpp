#include <scclient.h>
#include <signal.h>
#include <string.h>

#define ENDPOINT "qa-ws.stuffraiser.com"

volatile bool stop = false;

void logFunction(int level, const char *line)
{
    std::string log_str(line);
    std::cout << log_str;
}

void writerFunction(ScClient *client) {
    json_object *jobj = json_object_new_object();
    json_object *isCodeRunning = json_object_new_int(stop);
    json_object_object_add(jobj, "isCodeRunning", isCodeRunning);

    // The publish function frees the json object after it's sent.
    client->publish("test_channel", jobj);
}

void connected_callback(ScClient *client) {

}


int main()
{
    // If you want a log output other than the console from LWS.
    lws_set_log_level(7, &logFunction);

    signal(SIGINT, [](int) { stop = true; });

    ScClient *client = new ScClient(ENDPOINT, 80, "/socketcluster/");
    client->RegisterConnectedCallback(&connected_callback);
    client->connected_callback = std::function(connected_callback);

    while (!stop && client->socket_connect())
    {
        client->socket_reset();
        usleep(10000);
    }
    client->socket_disconnect();
}