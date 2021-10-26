
#ifndef __SCCLIENT_H__
#define __SCCLIENT_H__

#include <stdlib.h>
#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <signal.h>

#include "json-c/json.h"
#include <threadSafeQueue.h>
#include <threadSafeList.h>
#include <libwebsockets.h>

using namespace std;

static int ws_service_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);
struct session_data
{
    int fd;
    // void *user_space;
};

typedef function<void(string event, json_object *data)> sccCallback;
typedef tuple<string, sccCallback> subscription;


class ScClient
{
public:
    ScClient(string address, int port, string path);

    // Connection Functions
    int socket_connect();
    void socket_reset();
    void socket_disconnect();

    int handle_lws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

    // Send Functions
    void publish(string event, json_object *);
    void subscribe(string event, sccCallback);
    void unsubscribe(string event);

    // Acknowledge Functions
    std::function<void(int)> f_display;

    function<void(ScClient *)> connected_callback;
    function<void(string error)> connected_error_callback = NULL;
    function<void(string reason)> disconnected_callback = NULL;

    volatile bool connected;

    ~ScClient();

private:
    ThreadSafeList<subscription> *subscriptions;
    // map<string, sccCallback> subscriptions;

    ThreadSafeQueue<std::string> *message_queue;
    int msgCounter = 0;
    void message_processing();

    string address;
    int port;
    string path;

};

#endif // __SCCLIENT_H__