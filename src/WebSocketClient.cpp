/* =====================================================================================
 *
 * Filename:  WebSocketClient.cpp
 *
 * Description:  Implementation file.
 *
 * Version:  1.0
 * Created:  2025-11-29 09:49:14
 * Revision:  none
 * Compiler:  gcc / g++
 *
 * Author:  David P. Riedel <driedel@cox.net> with assist from Qwen3 and Gemini
 * Copyright (c) 2025, David P. Riedel
 *
 * =====================================================================================
 */
#include <iostream>

#include "WebSocketClient.h"

WebSocketClient::WebSocketClient(net::io_context &ioc, const std::string &host, const std::string &port,
                                 const std::string &target)
    : ioc_(ioc), ctx_(ssl::context::tlsv12_client), resolver_(ioc), ws_(ioc, ctx_), timer_(ioc), host_(host),
      port_(port), target_(target), should_stop_(false), connected_(false)
{
    ctx_.set_default_verify_paths();
}

void WebSocketClient::set_stop_callback(std::function<void()> callback)
{
    stop_callback_ = callback;
}

void WebSocketClient::run(std::chrono::seconds connect_timeout, std::chrono::seconds read_timeout)
{
    timer_.expires_after(connect_timeout);
    timer_.async_wait([this](boost::system::error_code ec) {
        if (!ec)
        {
            std::cout << "Connection timeout!\n";
            ws_.next_layer().next_layer().close();
        }
    });

    resolver_.async_resolve(host_, port_,
                            [this, read_timeout](boost::system::error_code ec, tcp::resolver::results_type results) {
                                on_resolve(ec, results, read_timeout);
                            });
}

void WebSocketClient::stop()
{
    should_stop_ = true;
    net::post(ioc_, [this]() {
        if (ws_.is_open())
        {
            ws_.close(websocket::close_code::normal);
        }
        connected_ = false;
        if (stop_callback_)
        {
            stop_callback_();
        }
    });
}

void WebSocketClient::send_message(const std::string &message)
{
    net::post(ioc_, [this, message]() {
        bool write_in_progress = !write_queue_.empty();
        write_queue_.push_back(message);
        if (!write_in_progress && connected_)
        {
            do_write();
        }
    });
}

void WebSocketClient::on_resolve(boost::system::error_code ec, tcp::resolver::results_type results,
                                 std::chrono::seconds read_timeout)
{
    if (ec)
    {
        std::cerr << "Resolve error: " << ec.message() << "\n";
        timer_.cancel();
        return;
    }

    // Pass the results object directly, not iterators
    net::async_connect(
        ws_.next_layer().next_layer(), results,
        [this, read_timeout](boost::system::error_code ec, const tcp::endpoint &) { on_connect(ec, read_timeout); });
}

void WebSocketClient::on_connect(boost::system::error_code ec, std::chrono::seconds read_timeout)
{
    if (ec)
    {
        std::cerr << "Connect error: " << ec.message() << "\n";
        timer_.cancel();
        return;
    }

    ws_.next_layer().async_handshake(ssl::stream_base::client, [this, read_timeout](boost::system::error_code ec) {
        on_ssl_handshake(ec, read_timeout);
    });
}

void WebSocketClient::on_ssl_handshake(boost::system::error_code ec, std::chrono::seconds read_timeout)
{
    if (ec)
    {
        std::cerr << "SSL Handshake error: " << ec.message() << "\n";
        timer_.cancel();
        return;
    }

    ws_.async_handshake(host_, target_,
                        [this, read_timeout](boost::system::error_code ec) { on_handshake(ec, read_timeout); });
}

void WebSocketClient::on_handshake(boost::system::error_code ec, std::chrono::seconds read_timeout)
{
    timer_.cancel();
    if (ec)
    {
        std::cerr << "Handshake error: " << ec.message() << "\n";
        return;
    }

    connected_ = true;
    std::cout << "Connected successfully!\n";
    std::cout << "Starting continuous read loop with " << read_timeout.count() << "s timeout per read\n";
    do_read(read_timeout);
}

void WebSocketClient::do_read(std::chrono::seconds read_timeout)
{
    if (should_stop_.load())
    {
        std::cout << "Stopping read loop\n";
        return;
    }

    timer_.expires_after(read_timeout);
    timer_.async_wait([this, read_timeout](boost::system::error_code ec) {
        if (!ec)
        {
            std::cout << "Read timeout!\n";
            ws_.next_layer().next_layer().close();
            if (stop_callback_)
            {
                stop_callback_();
            }
        }
    });

    ws_.async_read(buffer_, [this, read_timeout](boost::system::error_code ec, std::size_t bytes_transferred) {
        timer_.cancel();

        if (should_stop_.load())
        {
            return;
        }

        if (!ec)
        {
            std::cout << "Received " << bytes_transferred << " bytes: " << beast::buffers_to_string(buffer_.data())
                      << "\n";
            buffer_.consume(buffer_.size()); // Clear buffer
            do_read(read_timeout);
        }
        else
        {
            std::cout << "Read error: " << ec.message() << "\n";
            if (ec != boost::beast::websocket::error::closed)
            {
                stop();
            }
        }
    });
}

void WebSocketClient::do_write()
{
    ws_.async_write(
        net::buffer(write_queue_.front()),
        [this](boost::system::error_code ec, std::size_t bytes_transferred) { on_write(ec, bytes_transferred); });
}

void WebSocketClient::on_write(boost::system::error_code ec, std::size_t)
{
    if (ec)
    {
        std::cerr << "Write error: " << ec.message() << "\n";
        stop();
        return;
    }

    std::cout << "Message sent: " << write_queue_.front() << "\n";
    write_queue_.pop_front();

    if (!write_queue_.empty())
    {
        do_write();
    }
}
