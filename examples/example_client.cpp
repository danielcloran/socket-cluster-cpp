#include <scclient.h>
#include <signal.h>

#define ENDPOINT "qa-ws.stuffraiser.com"

volatile bool stop = false;


int main()
{

    ScClient *client = new ScClient(ENDPOINT, 80, "/socketcluster/");

    client->socket_connect();
    // while (!)
    // {
    //     client->socket_reset();
    //     usleep(10000);
    // }
    client->socket_disconnect();
}