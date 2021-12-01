//
//  SCClient.h
//
//  Created by Daniel Cloran on 11/2/21.
//

#ifndef SCClient_h
#define SCClient_h

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything" //ignore warnings in external libs

// Boost libs
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>

#pragma clang diagnostic pop

// Std libs
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "json-c/json.h"

// Callback and Subscription types
typedef std::function<void(std::string event, json_object *data)> socketCallback;
typedef std::tuple<std::string, socketCallback> subscription;


// Custom libs
#include "ThreadSafeQueue.h"
#include "ThreadSafeList.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class SCClient : public std::enable_shared_from_this<SCClient>
{
public:
    SCClient(net::io_context& ioc);
    std::thread launch_socket(const char * host, const char * port);
    void subscribe(std::string channel, socketCallback callback);
    void unsubscribe(std::string channel);
    void publish(std::string channel,  std::string data);
    void publish(std::string channel,  json_object *data);


    void stop();

private:
    void fail(beast::error_code ec, char const* what);
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
    void on_handshake(beast::error_code ec);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_close(beast::error_code ec);

    void run(char const* host, char const* port);

    // Async Message Processing
    int msgCounter = 1;
    std::thread messageThread;
    void message_processing();

    void resubscribe(std::string channel, socketCallback callback);

    net::io_context& the_io_context;
    tcp::resolver resolver_;
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string port_;

    std::condition_variable writable_cv;
    std::mutex writable_m;
    volatile bool writable = false;
    volatile bool socket_connected = false;
    volatile bool socket_closed = false;

    std::unique_ptr<ThreadSafeQueue<std::string>> message_queue;
    std::unique_ptr<ThreadSafeList<subscription>> subscription_list;
};

#endif /* SCClient_h */
