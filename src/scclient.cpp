#include <scclient.h>
#include <thread>

// Setting flags
static volatile bool destroy_flag = false;
static volatile bool connection_flag = false;

// These MUST be defined globally for LWS (will segfault if not)
struct lws_protocols protocol;
struct lws *wsi;
struct lws_context *context;
struct lws_context_creation_info info;
struct lws_client_connect_info i;
struct sigaction act;

ScClient::ScClient(string _address, int _port, string _path) : address(_address), port(_port), path(_path)
{
    message_queue = new ThreadSafeQueue<std::string>();
    subscriptions = new ThreadSafeList<subscription>();
}

void ScClient::message_processing()
{
    while (!destroy_flag)
    {
        message_queue->block_until_value();
        lws_callback_on_writable(wsi);
    }
}

int ScClient::socket_connect()
{
    context = NULL;
    wsi = NULL;

    signal(SIGINT, [](int)
           { destroy_flag = true; });

    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.iface = NULL;
    info.protocols = &protocol;
    info.ssl_cert_filepath = NULL;
    info.ssl_private_key_filepath = NULL;
    info.http_proxy_address = NULL;
    info.http_proxy_port = -1;
    static const struct lws_extension exts[] = {
        {"permessage-deflate", lws_extension_callback_pm_deflate, "permessage-deflate; client_max_window_bits"},
        {"deflate-frame", lws_extension_callback_pm_deflate, "deflate_frame"},
        {NULL, NULL, NULL},
    };
    info.extensions = exts;
    info.gid = -1;
    info.uid = -1;
    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    protocol.name = "websocket";
    protocol.callback = &ws_service_callback;
    protocol.per_session_data_size = sizeof(struct session_data);
    protocol.rx_buffer_size = 0;
    protocol.id = 0;

    context = lws_create_context(&info);

    memset(&i, 0, sizeof(i));
    i.port = port;
    i.path = path.c_str();
    i.address = address.c_str();
    i.context = context;
    i.ssl_connection = 0;
    i.host = address.c_str();
    i.origin = address.c_str();
    i.protocol = "websocket";
    i.ietf_version_or_minus_one = -1;
    i.userdata = (void *)this;

    lwsl_notice("[ScClient] context created.\n");

    if (context == NULL)
    {
        lwsl_notice("[ScClient] context is NULL.\n");
        return 0;
    }

    wsi = lws_client_connect_via_info(&i);

    if (wsi == NULL)
    {
        lwsl_notice("[ScClient] wsi create error.\n");
        return 0;
    }
    std::thread message_thread(&ScClient::message_processing, this);

    // lws_callback_on_writable(wsi);
    lwsl_notice("[ScClient] wsi create success.\n");

    while (!destroy_flag)
    {
        lws_service(context, 50);
    }
    lws_context_destroy(context);
    lwsl_notice("[ScClient] Socket Disconnected.\n");
    message_queue->enqueue((char *)"");
    message_thread.join();
    return 0;
}

void ScClient::socket_reset()
{
    lwsl_notice("[ScClient] Resetting socket server variables.");

    connection_flag = false;
    destroy_flag = false;

    // message_queue->clear();
}

void ScClient::socket_disconnect()
{
    destroy_flag = true;
    message_queue->enqueue((char *)"");
}

void ScClient::subscribe(string event, sccCallback f)
{
    std::shared_ptr<subscription> sub = subscriptions->find_first_if([event](subscription const &t)
                  { return std::get<0>(t) == event; });

    if (sub == nullptr)
    {
        json_object *jobj = json_object_new_object();
        json_object *eventobject = json_object_new_string("#subscribe");
        json_object *jobj1 = json_object_new_object();
        json_object *channelobject = json_object_new_string(event.c_str());
        json_object_object_add(jobj, "event", eventobject);
        json_object_object_add(jobj1, "channel", channelobject);
        json_object_object_add(jobj, "data", jobj1);
        json_object *cnt = json_object_new_int(++msgCounter);
        json_object_object_add(jobj, "cid", cnt);
        message_queue->enqueue(json_object_to_json_string(jobj));
        json_object_put(jobj);
        subscriptions->push_front(std::make_tuple(event, f));
        lwsl_notice("Subscribed to %s", event.c_str());
    }
    else
    {
        lwsl_notice("Duplicate Subscribe to %s attempted.", event.c_str());
    }
}

void ScClient::unsubscribe(string event)
{
    subscriptions->remove_if([event](subscription const &n)
                             { return std::get<0>(n) == event; });
    lwsl_notice("Unsubscribed from %s", event.c_str());
}

void ScClient::publish(string event, json_object *data)
{
    json_object *jobj = json_object_new_object();
    json_object *eventobject = json_object_new_string("#publish");
    json_object *jobj1 = json_object_new_object();
    json_object *cnt = json_object_new_int(++msgCounter);
    json_object *channelobject = json_object_new_string(event.c_str());
    json_object_object_add(jobj1, "channel", channelobject);
    json_object_object_add(jobj1, "data", data);
    json_object_object_add(jobj, "event", eventobject);
    json_object_object_add(jobj, "data", jobj1);
    json_object_object_add(jobj, "cid", cnt);

    message_queue->enqueue((char *)json_object_to_json_string(jobj));
    json_object_put(jobj);
}

int ScClient::handle_lws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    // printf("INSIDE: printf(\"%%p\", pf) is %p\n", this);

    // lwsl_notice("handle_lws_callback reason: %d", reason);
    switch (reason)
    {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
    {
        lwsl_notice("[ScClient Service] Connect with server success.\n");
        json_object *jobj = json_object_new_object();
        json_object *event = json_object_new_string("#handshake");
        json_object *cid = json_object_new_int(++msgCounter);

        json_object_object_add(jobj, "event", event);
        json_object_object_add(jobj, "data", NULL);
        json_object_object_add(jobj, "cid", cid);

        message_queue->enqueue((char *)json_object_to_json_string(jobj));

        lwsl_notice("[ScClient Service] Connected to server.\n");

        lws_callback_on_writable(wsi);
        connection_flag = 1;

        // Resubscribe to channels after reconnect
        ThreadSafeList<subscription> *resubscriptions = subscriptions;
        subscriptions = new ThreadSafeList<subscription>();
        resubscriptions->for_each([this](subscription const &n)
                                  { subscribe(get<0>(n), get<1>(n)); });
        delete resubscriptions;

        json_object_put(jobj);
        if (connected_callback)
        {
            connected_callback(this);
        }
    }
    break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    {
        lwsl_notice("[ScClient Service] Connect with server error.\n");
        destroy_flag = 1;
        connection_flag = 0;
        if (connected_error_callback != NULL)
        {
            connected_error_callback("LWS Error.");
        }
    }
    break;

    case LWS_CALLBACK_CLOSED:
    {
        lwsl_notice("[ScClient Service] LWS_CALLBACK_CLOSED\n");
        destroy_flag = 1;
        connection_flag = 0;
        if (disconnected_callback != NULL)
        {
            disconnected_callback("LWS Error.");
        }
    }
    break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
    {
        // Keep Alive, ping/pong.
        if (strcmp((char *)in, ping_str.c_str()) == 0)
        {
            message_queue->enqueue(pong_str.c_str());
        }
        else
        {
            json_object *jobj = json_tokener_parse((char *)in);
            if (json_object_get_type(jobj) != json_type_object)
            {
                lwsl_notice("[ScClient Service] data received is either null or not json parsable.\n");
                break;
            }
            json_object *msgData;
            int exists = json_object_object_get_ex(jobj, "data", &msgData);
            if (exists)
            {
                json_object *data = json_object_new_object();
                string channel = "";
                json_object_object_foreach(msgData, key, val)
                {
                    if (strcmp("channel", key) == 0)
                    {
                        channel = json_object_get_string(val);
                    }
                    if (strcmp("data", key) == 0)
                    {
                        data = val;
                    }
                }

                if (!channel.empty())
                {
                    std::shared_ptr<subscription> sub = subscriptions->find_first_if([channel](subscription const &t)
                                                        { return std::get<0>(t) == channel; });
                    if (sub != nullptr)
                    {
                        get<1>(*sub)(channel, data);
                    }
                }
                json_object_put(data);
            }
        }
    }
    break;
    case LWS_CALLBACK_CLIENT_WRITEABLE:
    {
        std::string message = message_queue->dequeue();

        if (message != "empty")
        {
            // std::cout << "Sending: " << message << std::endl;
            unsigned char *output = (unsigned char *)malloc(sizeof(unsigned char) * (LWS_SEND_BUFFER_PRE_PADDING + message.size() + LWS_SEND_BUFFER_POST_PADDING));
            memcpy(output + LWS_SEND_BUFFER_PRE_PADDING * sizeof(unsigned char), message.c_str(), message.size());
            lws_write(wsi, output + LWS_SEND_BUFFER_PRE_PADDING * sizeof(unsigned char), message.size(), LWS_WRITE_TEXT);
            free(output);
        }
    }
    break;
    case LWS_CALLBACK_WSI_DESTROY:
    {
        lwsl_notice("[ScClient Service] LWS_CALLBACK_WSI_DESTROY\n");
        destroy_flag = 1;
    }
    default:
        break;
    }
    return 0;
}

static int ws_service_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{

    if (user != NULL)
    {
        ScClient *client = static_cast<ScClient *>(user);
        return client->handle_lws_callback(wsi, reason, user, in, len);
    }
    return 0;
}

ScClient::~ScClient()
{
    delete message_queue;
    delete subscriptions;
}