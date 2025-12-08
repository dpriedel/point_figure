// =====================================================================================
//       Filename:  Streamer.cpp
//    Description:  Wrapper for streaming data providers (Async Implementation)
// =====================================================================================

#include "Streamer.h"
#include <boost/asio/bind_executor.hpp>

namespace rng = std::ranges;
namespace http = beast::http;

RemoteDataSource::RemoteDataSource() : ctx_{ssl::context::tlsv12_client}, resolver_{ioc_}, ws_{ioc_, ctx_}
{
}

RemoteDataSource::RemoteDataSource(const Host &host, const Port &port, const APIKey &api_key, const Prefix &prefix)
    : api_key_{api_key.get()}, host_{host.get()}, port_{port.get()}, websocket_prefix_{prefix.get()},
      ctx_{ssl::context::tlsv12_client}, resolver_{ioc_}, ws_{ioc_, ctx_}
{
}

RemoteDataSource::~RemoteDataSource()
{
    // Ensure IOC is stopped if object is destroyed
    if (!ioc_.stopped())
        ioc_.stop();
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
    net::async_connect(
        beast::get_lowest_layer(ws_), results, beast::bind_front_handler(&RemoteDataSource::on_connect, this));
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
        return;
    }

    ws_.set_option(beast::websocket::stream_base::timeout::suggested(beast::role_type::client));
    ws_.set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
        req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async");
    }));

    // 4. Websocket Handshake
    std::string tmp_host = host_ + ':' + port_; // Port is often implied in host string for handshake, or use host
    ws_.async_handshake(host_, websocket_prefix_, beast::bind_front_handler(&RemoteDataSource::on_handshake, this));
}

void RemoteDataSource::on_handshake(beast::error_code ec)
{
    if (ec)
    {
        spdlog::error("Websocket Handshake failed: {}", ec.message());
        return;
    }

    spdlog::debug("Connected. performing subscription...");
    // 5. Let derived class handle subscription
    OnConnected();
}

void RemoteDataSource::StartReadLoop()
{
    // Called by derived class when subscription is complete
    spdlog::debug("Subscription complete. Starting read loop.");
    do_read();
}

void RemoteDataSource::do_read()
{
    // Set timer for read timeout (e.g., 30 seconds)
    timer_.expires_after(std::chrono::seconds(30));
    timer_.async_wait(beast::bind_front_handler(&RemoteDataSource::on_timer, this));

    // Async Read
    buffer_.clear();
    ws_.async_read(buffer_, beast::bind_front_handler(&RemoteDataSource::on_read, this));
}

void RemoteDataSource::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    // Operation completed, cancel timer
    timer_.cancel();

    if (ec == websocket::error::closed)
    {
        spdlog::debug("Websocket closed normally.");
        StopStreaming(*context_ptr_);
        return;
    }
    if (ec)
    {
        spdlog::error("Read error: {}", ec.message());
        StopStreaming(*context_ptr_);
        return;
    }

    // Process Data
    std::string buffer_content = beast::buffers_to_string(buffer_.cdata());
    if (!buffer_content.empty() && context_ptr_)
    {
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
    if (ec != net::error::operation_aborted)
    {
        // Timer fired implies timeout
        spdlog::error("Stream read timeout. Closing connection.");
        // Closing the socket will cause on_read to exit with operation_aborted or similar error
        ws_.next_layer().next_layer().close();
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

        // Stop the IO context to exit the run() loop
        ioc_.stop();
    }
}

void RemoteDataSource::RequestStop()
{
    // This can be called from outside to trigger graceful shutdown
    boost::asio::post([this]() {
        if (context_ptr_)
            StopStreaming(*context_ptr_);
    });
}
void RemoteDataSource::DisconnectWS()
{
    // Cancel any pending operations
    timer_.cancel();

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
                // Access the underlying TCP socket through the SSL stream
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
