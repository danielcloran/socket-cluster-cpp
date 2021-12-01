//
//  SCClient.cpp
//
//  Created by Daniel Cloran on 11/2/21.
//

#include "SCClient.h"

SCClient::SCClient(net::io_context& ioc) : resolver_(net::make_strand(ioc)), ws_(net::make_strand(ioc)), the_io_context(ioc) {
    message_queue = std::make_unique<ThreadSafeQueue<std::string>>();
    subscription_list = std::make_unique<ThreadSafeList<subscription>>();

    messageThread = std::thread(&SCClient::message_processing, this);
    messageThread.detach();
}

std::thread SCClient::launch_socket(const char * host, const char * port)
{
    std::thread socketThread([this, host, port]
    {
        while (!socket_closed)
        {
            run(host, port);
            // Run the I/O service. The call will return when
            // the socket is closed.
            the_io_context.run();

            // set to false as safety in case ::fail wasn't called.
            socket_connected = false;
            the_io_context.reset(); // reset the socket
        }
    });
    while (!socket_connected)
    {
        // Busy wait until socket has connected for the first time
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return socketThread;
}

// Inefficient, but it works.
// Otherwise we'd have to make a copy to avoid freein the users json_object
void SCClient::publish(std::string channel,  json_object *data) {
    publish(channel, json_object_to_json_string(data));
}

void SCClient::publish(std::string channel,  std::string data) {
    json_object *dataObj = json_tokener_parse((char *)data.c_str());
    json_object *jobj = json_object_new_object();
    json_object *eventobject = json_object_new_string("#publish");
    json_object *jobj1 = json_object_new_object();
    json_object *cnt = json_object_new_int(++msgCounter);
    json_object *channelobject = json_object_new_string(channel.c_str());
    json_object_object_add(jobj1, "channel", channelobject);
    json_object_object_add(jobj1, "data", dataObj);
    json_object_object_add(jobj, "event", eventobject);
    json_object_object_add(jobj, "data", jobj1);
    json_object_object_add(jobj, "cid", cnt);
    message_queue->enqueue((char *)json_object_to_json_string(jobj));
    json_object_put(jobj);
}

void SCClient::subscribe(std::string channel, socketCallback callback) {
    std::shared_ptr<subscription> sub = subscription_list->find_first_if([channel](subscription const &t)
                      { return std::get<0>(t) == channel; });

    if (sub == nullptr)
    {
        json_object *jobj = json_object_new_object();
        json_object *eventobject = json_object_new_string("#subscribe");
        json_object *jobj1 = json_object_new_object();
        json_object *channelobject = json_object_new_string(channel.c_str());
        json_object_object_add(jobj, "event", eventobject);
        json_object_object_add(jobj1, "channel", channelobject);
        json_object_object_add(jobj, "data", jobj1);
        json_object *cnt = json_object_new_int(++msgCounter);
        json_object_object_add(jobj, "cid", cnt);
        message_queue->enqueue(json_object_to_json_string(jobj));
        json_object_put(jobj);
        subscription_list->push_front(std::make_tuple(channel, callback));
        std::cout << "Subscribed to " << channel << std::endl;
    }
    else
    {
        std::cout << "Duplicate Subscribe to " << channel << " attempted" << std::endl;
    }
}

void SCClient::resubscribe(std::string channel, socketCallback callback) {
    json_object *jobj = json_object_new_object();
    json_object *eventobject = json_object_new_string("#subscribe");
    json_object *jobj1 = json_object_new_object();
    json_object *channelobject = json_object_new_string(channel.c_str());
    json_object_object_add(jobj, "event", eventobject);
    json_object_object_add(jobj1, "channel", channelobject);
    json_object_object_add(jobj, "data", jobj1);
    json_object *cnt = json_object_new_int(++msgCounter);
    json_object_object_add(jobj, "cid", cnt);
    message_queue->enqueue(json_object_to_json_string(jobj));
    json_object_put(jobj);
    std::cout << "Resubscribed to " << channel << std::endl;
}

void SCClient::unsubscribe(std::string channel) {
    subscription_list->remove_if([channel](subscription const &n)
                                 { return std::get<0>(n) == channel; });
    std::cout << "Unsubscribed from " << channel << std::endl;
}

// Report a failure
void SCClient::fail(beast::error_code ec, char const* what)
{
    socket_connected = false;
    std::cerr << "Socket Fail / Closing: " << what << ": " << ec.message() << "\n";
}

// Start the asynchronous operation
void SCClient::run(char const* host, char const* port)
{
    writable = false;
    host_ = host;
    port_ = port;

    // Look up the domain name
    resolver_.async_resolve(host, port, beast::bind_front_handler(&SCClient::on_resolve, shared_from_this()));
}

void SCClient::stop() {
    // Close the WebSocket connection
    socket_closed = true;
    ws_.async_close(websocket::close_code::normal, beast::bind_front_handler(&SCClient::on_close, shared_from_this()));
}

void SCClient::on_close(beast::error_code ec)
{
    if(ec) return fail(ec, "close");
    std::string s(boost::asio::buffer_cast<const char*>(buffer_.data()), buffer_.size());
}

void SCClient::on_resolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if(ec) return fail(ec, "resolve");

    // Set the timeout for the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(5));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(ws_).async_connect(results, beast::bind_front_handler(&SCClient::on_connect, shared_from_this()));
}

void SCClient::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
{
    if(ec) return fail(ec, "connect");

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(ws_).expires_never();

    // Set suggested timeout settings for the websocket
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

    // Set a decorator to change the User-Agent of the handshake
    ws_.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-async");
        }));

    host_ += ':' + std::to_string(ep.port());
    // Perform the websocket handshake
    ws_.async_handshake(host_, "/socketcluster/", beast::bind_front_handler(&SCClient::on_handshake, shared_from_this()));

    std::cout << "Successfully Connected to Socket." << std::endl;
}

void SCClient::on_handshake(beast::error_code ec)
{
    if(ec)
        return fail(ec, "handshake");

    message_queue->push_front("{\"event\":\"#handshake\",\"data\":{\"authToken\":null},\"cid\": 1 }");
    msgCounter = 1;
    std::unique_lock<std::mutex> ul(writable_m);
    writable = true;
    writable_cv.notify_one();

    socket_connected = true;
    // Kick off Async Reading.
    // Read a message into our buffer
    ws_.async_read(buffer_, beast::bind_front_handler(&SCClient::on_read, shared_from_this()));

    // Resubscribe to all channels after reconnect
    subscription_list->for_each([this](subscription const &n)
                              {resubscribe(std::get<0>(n), std::get<1>(n));});
}

// Monitor the ThreadSafeQueue and call write when writable
void SCClient::message_processing() {
    while (true) {
        /*
            Only one call to async_write is allowed at a time.
            on_write (callback of async_write) sets writable = true again.
        */

        message_queue->block_until_value();
        std::unique_lock<std::mutex> ul(writable_m);
        writable_cv.wait(ul,[this] {return writable;});

        std::string message = message_queue->dequeue();
        writable = false;

        // Post into the main processing thread
        net::post(the_io_context, [this, message]
        {
            ws_.async_write(net::buffer(message), beast::bind_front_handler(&SCClient::on_write, shared_from_this()));
            // std::cout << "Wrote: " << message << std::endl;
         });
    }
}

void SCClient::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if(ec)
        return fail(ec, "write");

    std::unique_lock<std::mutex> ul(writable_m);  //lock is applied on mutex m by thread t2
    writable = true;
    writable_cv.notify_one();  //notify to condition variable
}

void SCClient::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if(ec) return fail(ec, "read");
    if (bytes_transferred == 0)
    {
        message_queue->enqueue("");
    }
    else
    {
        std::string s(boost::asio::buffer_cast<const char*>(buffer_.data()), buffer_.size());
        json_object *jobj = json_tokener_parse((char *)s.c_str());
        if (json_object_get_type(jobj) != json_type_object)
        {
            std::cout << "[ScClient Service] data received is either null or not json parsable." << std::endl;
        }
        else
        {
            json_object *msgData;
            int exists = json_object_object_get_ex(jobj, "data", &msgData);
            if (exists)
            {
                json_object *data = json_object_new_object();
                std::string channel = "";
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
                    std::shared_ptr<subscription> sub = subscription_list->find_first_if([channel](subscription const &t)
                                                        { return std::get<0>(t) == channel; });
                    if (sub != nullptr)
                    {
                        std::get<1>(*sub)(channel, data);
                    }
                }
            }
            json_object_put(jobj);
        }
    }

    buffer_.clear();
    // Async_read recursively calls itself, so kick off once here.
    ws_.async_read(buffer_, beast::bind_front_handler(&SCClient::on_read, shared_from_this()));
}
