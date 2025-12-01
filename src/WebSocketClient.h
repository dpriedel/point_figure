/* =====================================================================================
 *
 * Filename:  WebSocketClient.h
 *
 * Description:  WebSocketClient based on code from Qwen3 and Gemini
 *
 *
 * Version:  1.0
 * Created:  2025-11-29 09:43:48
 * Revision:  none
 * Compiler:  gcc / g++
 *
 * Author:  David P. Riedel <driedel@cox.net> with assist from Qwen3 and Gemini
 * Copyright (c) 2025, David P. Riedel
 *
 * =====================================================================================
 */

#ifndef WEBSOCKETCLIENT_H_
#define WEBSOCKETCLIENT_H_

#include <atomic>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <deque>
#include <functional>
// #include <memory>
#include <string>

namespace beast = boost::beast;
// namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

class WebSocketClient
{
private:
    net::io_context &ioc_;
    ssl::context ctx_;
    tcp::resolver resolver_;
    websocket::stream<ssl::stream<tcp::socket>> ws_;
    net::steady_timer timer_;
    std::string host_;
    std::string port_;
    std::string target_;
    std::atomic<bool> should_stop_;
    std::atomic<bool> connected_;
    std::function<void()> stop_callback_;
    beast::flat_buffer buffer_;
    std::deque<std::string> write_queue_;

public:
    WebSocketClient(net::io_context &ioc, const std::string &host, const std::string &port, const std::string &target);

    void set_stop_callback(std::function<void()> callback);

    void run(std::chrono::seconds connect_timeout, std::chrono::seconds read_timeout);

    void stop();

    bool is_connected() const
    {
        return connected_.load();
    }

    void send_message(const std::string &message);

private:
    void on_resolve(boost::system::error_code ec, tcp::resolver::results_type results,
                    std::chrono::seconds read_timeout);

    void on_connect(boost::system::error_code ec, std::chrono::seconds read_timeout);

    void on_ssl_handshake(boost::system::error_code ec, std::chrono::seconds read_timeout);

    void on_handshake(boost::system::error_code ec, std::chrono::seconds read_timeout);

    void do_read(std::chrono::seconds read_timeout);

    void do_write();

    void on_write(boost::system::error_code ec, std::size_t);
};

class SignalHandler
{
private:
    net::io_context &ioc_;
    net::steady_timer timer_;

public:
    SignalHandler(net::io_context &ioc) : ioc_(ioc), timer_(ioc)
    {
    }

    void setup_stop_timer(std::chrono::seconds delay, std::function<void()> stop_callback)
    {
        timer_.expires_after(delay);
        timer_.async_wait([stop_callback](boost::system::error_code) {
            if (stop_callback)
            {
                stop_callback();
            }
        });
    }
};

#endif /* WEBSOCKETCLIENT_H_ */
