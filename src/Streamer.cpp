// =====================================================================================
//       Filename:  Streamer.cpp
//    Description:  Wrapper for streaming data providers (Async Implementation)
// =====================================================================================

#include "Streamer.h"
#include <algorithm>
#include <boost/asio/bind_executor.hpp>

namespace rng = std::ranges;
namespace http = beast::http;

RemoteDataSource::RemoteDataSource()
    : ctx_{ssl::context::tlsv12_client}, resolver_{ioc_}, ws_{ioc_, ctx_}, reconnect_timer_(ioc_),
      max_reconnect_attempts_(5), reconnect_attempts_(0), base_reconnect_delay_(std::chrono::seconds(1)),
      rng_(std::random_device{}()), jitter_dist_(0, 500), should_reconnect_(false)
{
}

RemoteDataSource::RemoteDataSource(const Host &host, const Port &port, const APIKey &api_key, const Prefix &prefix)
    : api_key_{api_key.get()}, host_{host.get()}, port_{port.get()}, websocket_prefix_{prefix.get()},
      ctx_{ssl::context::tlsv12_client}, resolver_{ioc_}, ws_{ioc_, ctx_}, reconnect_timer_(ioc_),
      max_reconnect_attempts_(5), reconnect_attempts_(0), base_reconnect_delay_(std::chrono::seconds(1)),
      rng_(std::random_device{}()), jitter_dist_(0, 500), should_reconnect_(false)
{
}

RemoteDataSource::~RemoteDataSource()
{
    // Ensure IOC is stopped if object is destroyed
    if (!ioc_.stopped())
        ioc_.stop();
}

void RemoteDataSource::StartReconnect()
{
    should_reconnect_ = true;
    reconnect_attempts_ = 0; // Reset attempts
    start_reconnection();
}

void RemoteDataSource::StreamData(bool *had_signal, StreamerContext &streamer_context)
{
    // Store pointers for async handlers
    had_signal_ptr_ = had_signal;
    context_ptr_ = &streamer_context;
    *had_signal = false;

    // Reset io_context for new run
    ioc_.restart();

    // Setup Signal Handling (Ctrl-C)
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
    signals_.async_wait(beast::bind_front_handler(&RemoteDataSource::on_signal, this));

    // Start Connection Process
    ConnectWS();

    // Run the IO context - this blocks until the stream stops or is interrupted
    try
    {
        ioc_.run();
    }
    catch (const std::exception &e)
    {
        spdlog::error("Exception in StreamData loop: {}", e.what());
        *had_signal = true;
    }

    // Cleanup
    signals_.clear();
    context_ptr_ = nullptr;
    had_signal_ptr_ = nullptr;
}

void RemoteDataSource::ConnectWS()
{
    // 1. Resolve
    resolver_.async_resolve(host_, port_, beast::bind_front_handler(&RemoteDataSource::on_resolve, this));
}

void RemoteDataSource::on_resolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if (ec)
    {
        spdlog::error("Resolve failed: {}", ec.message());
        return;
    }

    // 2. Connect TCP
    net::async_connect(beast::get_lowest_layer(ws_), results,
                       beast::bind_front_handler(&RemoteDataSource::on_connect, this));
}

void RemoteDataSource::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
{
    if (ec)
    {
        spdlog::error("Connect failed: {}", ec.message());
        return;
    }

    if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str()))
    {
        spdlog::error("Failed to set SNI Hostname");
        return;
    }

    // 3. SSL Handshake
    ws_.next_layer().async_handshake(ssl::stream_base::client,
                                     beast::bind_front_handler(&RemoteDataSource::on_ssl_handshake, this));
}

void RemoteDataSource::on_ssl_handshake(beast::error_code ec)
{
    if (ec)
    {
        spdlog::error("SSL Handshake failed: {}", ec.message());
        should_reconnect_ = true;
        start_reconnection();
        return;
    }

    beast::websocket::stream_base::timeout opt{
        std::chrono::seconds(30), // Handshake timeout
        std::chrono::seconds(30), // Idle timeout (disconnect if no data/pong for 30s)
        true                      // Keep-Alive: Send a Ping if idle
    };
    ws_.set_option(opt);

    ws_.set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
        req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async");
    }));

    ws_.async_handshake(host_, websocket_prefix_, beast::bind_front_handler(&RemoteDataSource::on_handshake, this));
}

void RemoteDataSource::on_handshake(beast::error_code ec)
{
    if (ec)
    {
        spdlog::error("Websocket Handshake failed: {}", ec.message());
        if (should_reconnect_)
        {
            start_reconnection();
        }
        return;
    }

    spdlog::debug("Connected. performing subscription...");
    reconnect_attempts_ = 0;  // Reset attempts on successful connection
    should_reconnect_ = true; // Enable reconnection from now on
    // 5. Let derived class handle subscription
    OnConnected();
}

void RemoteDataSource::start_reconnection()
{
    if (!should_reconnect_ || reconnect_attempts_ >= max_reconnect_attempts_)
    {
        spdlog::info("Max reconnection attempts reached or reconnection disabled. Stopping.");
        ioc_.stop();
        return;
    }

    ++reconnect_attempts_;
    auto delay = calculate_reconnect_delay();

    spdlog::info("Attempting to reconnect in {} seconds (attempt {}/{}).", delay.count(), reconnect_attempts_,
                 max_reconnect_attempts_);

    reconnect_timer_.expires_after(delay);
    reconnect_timer_.async_wait([this](beast::error_code ec) {
        if (ec == net::error::operation_aborted)
        {
            return;
        }

        if (ec)
        {
            spdlog::error("Reconnection timer error: {}", ec.message());
            return;
        }

        try_reconnect();
    });
}

std::chrono::milliseconds RemoteDataSource::calculate_reconnect_delay()
{

    auto delay = base_reconnect_delay_ * std::pow(2.0, reconnect_attempts_ - 1);
    auto jitter = std::chrono::milliseconds(jitter_dist_(rng_));
    delay += jitter;
    auto max_delay = std::chrono::seconds(30);
    // Convert both to milliseconds for comparison
    auto delay_ms = std::chrono::duration_cast<std::chrono::milliseconds>(delay);
    auto max_delay_ms = std::chrono::duration_cast<std::chrono::milliseconds>(max_delay);
    return std::min(delay_ms, max_delay_ms);
}

void RemoteDataSource::try_reconnect()
{
    try
    {
        // Close any existing connection
        if (ws_.is_open())
        {
            // ws_.close(websocket::close_code::normal);
            beast::get_lowest_layer(ws_).close();
        }

        // Reconnect
        ConnectWS();
    }
    catch (const std::exception &e)
    {
        spdlog::error("Reconnection attempt failed: {}", e.what());
        start_reconnection(); // Try again
    }
}

void RemoteDataSource::StartReadLoop()
{
    // Called by derived class when subscription is complete
    spdlog::debug("Subscription complete. Starting read loop.");

    // make sure there are no left-overs in the buffer from subscription Process.

    buffer_.clear();
    do_read();
}

void RemoteDataSource::do_read()
{
    // Async Read
    ws_.async_read(buffer_, beast::bind_front_handler(&RemoteDataSource::on_read, this));
}

void RemoteDataSource::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    if (ec == websocket::error::closed)
    {
        spdlog::debug("Websocket closed normally.");
        StopStreaming(*context_ptr_);
        if (had_signal_ptr_)
            *had_signal_ptr_ = true;
        return;
    }

    // Handle Timeouts (detected by Beast via missed Ping/Pong)
    // Beast uses net::error::timed_out for websocket timeouts
    if (ec == beast::error::timeout)
    {
        spdlog::warn("WebSocket connection timed out (no Ping/Pong). Reconnecting.");
        should_reconnect_ = true;
        start_reconnection();
        return;
    }

    if (ec == boost::asio::error::eof)
    {
        spdlog::warn("Remote socket closed unexpectedly. Attempting reconnection.");
        should_reconnect_ = true;
        start_reconnection();
        return;
    }

    if (ec)
    {
        spdlog::error("Read error: {}", ec.message());
        if (should_reconnect_)
        {
            start_reconnection();
        }
        else
        {
            StopStreaming(*context_ptr_);
            if (had_signal_ptr_)
                *had_signal_ptr_ = true;
        }
        return;
    }

    // Process Data
    if (buffer_.size() > 0 && context_ptr_)
    {
        // We read the data, then manually consume it.
        std::string buffer_content = beast::buffers_to_string(buffer_.cdata());

        // Remove the processed bytes from the buffer so it's empty for the next read
        buffer_.clear();

        {
            std::lock_guard<std::mutex> queue_lock(context_ptr_->mtx_);
            context_ptr_->streamed_data_.push(std::move(buffer_content));
        }
        context_ptr_->cv_.notify_one();
    }

    // Loop
    if (had_signal_ptr_ && *had_signal_ptr_)
    {
        RequestStop();
    }
    else
    {
        do_read();
    }
}

void RemoteDataSource::on_timer(beast::error_code ec)
{
    if (ec == net::error::operation_aborted)
        return; // Timer was cancelled, do nothing

    spdlog::error("Stream read timeout. Closing connection.");

    // Just close the websocket. This triggers the read handler with an error.
    // Don't mess with lowest_layer directly unless WS close fails.
    if (ws_.is_open())
    {
        ws_.next_layer().next_layer().cancel(); // Cancel TCP ops
        ws_.async_close(websocket::close_code::policy_error, [](beast::error_code) {});
    }
}

void RemoteDataSource::on_signal(beast::error_code ec, int signal_number)
{
    if (!ec)
    {
        spdlog::info("Signal {} received. Stopping...", signal_number);
        if (had_signal_ptr_)
            *had_signal_ptr_ = true;
        if (context_ptr_)
            StopStreaming(*context_ptr_);

        should_reconnect_ = false; // Disable reconnection
        // Stop the IO context to exit the run() loop
        ioc_.stop();
    }
}

void RemoteDataSource::RequestStop()
{
    // This can be called from outside to trigger graceful shutdown
    boost::asio::post(ioc_, [this]() {
        if (had_signal_ptr_)
            *had_signal_ptr_ = true;
        if (context_ptr_)
            StopStreaming(*context_ptr_);
    });
}
void RemoteDataSource::DisconnectWS()
{
    // Cancel any pending operations
    timer_.cancel();
    reconnect_timer_.cancel();

    if (ws_.is_open())
    {
        // Close WebSocket gracefully
        ws_.async_close(websocket::close_code::normal, [this](beast::error_code ec) {
            if (ec)
            {
                spdlog::debug("WebSocket close error: {}", ec.message());
            }
            // Close underlying socket properly
            try
            {
                auto &socket = beast::get_lowest_layer(ws_);
                if (socket.is_open())
                {
                    socket.shutdown(tcp::socket::shutdown_both);
                    socket.close();
                }
            }
            catch (const std::exception &e)
            {
                spdlog::debug("Socket close error: {}", e.what());
            }
            // Stop the io_context to exit the run loop
            ioc_.stop();
        });
    }
    else
    {
        // WebSocket already closed, just stop the io_context
        ioc_.stop();
    }
}

void RemoteDataSource::UseSymbols(const std::vector<std::string> &symbols)
{
    symbol_list_ = symbols;
    rng::for_each(symbol_list_, [](auto &symbol) { rng::for_each(symbol, [](char &c) { c = std::toupper(c); }); });
}

std::string RemoteDataSource::RequestData(const std::string &request_string)
{
    // Keep this synchronous for one-off requests
    net::io_context ioc_req;
    ssl::context ctx_req{ssl::context::tlsv12_client};
    beast::ssl_stream<beast::tcp_stream> stream(ioc_req, ctx_req);
    tcp::resolver resolver_req{ioc_req};

    if (!SSL_set_tlsext_host_name(stream.native_handle(), host_.c_str()))
        throw beast::system_error{
            beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category())};

    auto const results = resolver_req.resolve(host_, port_);
    beast::get_lowest_layer(stream).connect(results);
    stream.handshake(ssl::stream_base::client);

    http::request<http::string_body> req{http::verb::get, request_string, version};
    req.set(http::field::host, host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    http::write(stream, req);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    beast::get_lowest_layer(stream).close();
    return res.body();
}
