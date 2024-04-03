// =====================================================================================
//
//       Filename:  Streamer.cpp
//
//    Description:  Wrapper for streaming data providers
//
//        Version:  2.0
//        Created:  2024-04-01 09:23 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//   Organization:
//
// =====================================================================================

#include "Streamer.h"

namespace rng = std::ranges;
namespace http = beast::http;

RemoteDataSource::RemoteDataSource()  // constructor
    : ctx{ssl::context::tlsv12_client}, resolver{ioc}, ws{ioc, ctx}
{
}
RemoteDataSource::RemoteDataSource(const Host& host, const Port& port, const APIKey& api_key, const Prefix& prefix)
    : api_key{api_key.get()},
      host{host.get()},
      port{port.get()},
      websocket_prefix{prefix.get()},
      ctx{ssl::context::tlsv12_client},
      resolver{ioc},
      ws{ioc, ctx}
{
}

RemoteDataSource::~RemoteDataSource()
{
    // need to disconnect if still connected.

    DisconnectWS();
}

void RemoteDataSource::RemoteDataSource::ConnectWS()
{
    // the following code is taken from example in Boost documentation

    auto const results = resolver.resolve(host, port);

    auto ep = net::connect(get_lowest_layer(ws), results);

    if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str()))
    {
        throw beast::system_error(
            beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()),
            "Failed to set SNI Hostname");
    }

    auto tmp_host = host + ':' + std::to_string(ep.port());

    ws.set_option(beast::websocket::stream_base::timeout::suggested(beast::role_type::client));

    // Perform the SSL handshake
    ws.next_layer().handshake(ssl::stream_base::client);

    // Perform the websocket handshake
    ws.handshake(tmp_host, websocket_prefix);
    BOOST_ASSERT_MSG(ws.is_open(), "Unable to complete websocket connection.");
}

void RemoteDataSource::DisconnectWS()
{
    if (!ws.is_open())
    {
        return;
    }
    try
    {
        // beast::close_socket(get_lowest_layer(ws_));
        ws.close(websocket::close_code::normal);
    }
    catch (std::exception& e)
    {
        spdlog::error("Problem closing socket during disconnect: {}.", e.what());
    }
}
void RemoteDataSource::StreamData(bool* had_signal, std::mutex* data_mutex, std::queue<std::string>* streamed_data)
{
    StartStreaming();

    // This buffer will hold the incoming message
    // Each message contains data for a single transaction

    beast::flat_buffer buffer;

    while (ws.is_open() && !(*had_signal))
    {
        try
        {
            buffer.clear();
            ws.read(buffer);
            ws.text(ws.got_text());
            std::string buffer_content = beast::buffers_to_string(buffer.cdata());
            if (!buffer_content.empty())
            {
                // std::cout << buffer_content << std::endl;
                // ExtractData(buffer_content);
                const std::lock_guard<std::mutex> queue_lock(*data_mutex);
                streamed_data->push(std::move(buffer_content));
            }
        }
        catch (std::system_error& e)
        {
            // any system problems, we close the socket and force our way out.
            // on EOF, just cleanly shutdown. Let higher level code
            // decide what to do then.

            auto ec = e.code();
            if (ec.value() == boost::asio::error::eof)
            {
                spdlog::info("EOF on websocket read. Exiting streaming.");
                StopStreaming();
                throw StreamingEOF{};
            }
            spdlog::error(std::format("System error. Category: {}. Value: {}. Message: {}", ec.category().name(),
                                      ec.value(), ec.message()));
            *had_signal = true;
            ws.close(websocket::close_code::going_away);
        }
        catch (std::exception& e)
        {
            spdlog::error(std::format("Problem processing steamed data. Message: {}", e.what()));

            if (std::string_view{e.what()}.starts_with("End of file") ||
                std::string_view{e.what()}.starts_with("End of stream"))
            {
                // I had expected to get a system error here. Hence the
                // catch block above.
                // Anyways, let's just shutdown the stream and exit.

                spdlog::info("EOF on websocket read. Exiting streaming.");
                StopStreaming();
                throw StreamingEOF{};
            }
            *had_signal = true;
            StopStreaming();
        }
        catch (...)
        {
            spdlog::error("Unknown problem processing steamed data.");
            *had_signal = true;
            StopStreaming();
        }
    }
    if (*had_signal)
    {
        // do a last check for data

        buffer.clear();
        ws.read(buffer);
        std::string buffer_content = beast::buffers_to_string(buffer.cdata());
        if (!buffer_content.empty())
        {
            const std::lock_guard<std::mutex> queue_lock(*data_mutex);
            streamed_data->push(std::move(buffer_content));
        }
    }
    // if the websocket is closed on the server side or there is a timeout which in turn
    // will cause the websocket to be closed, let's set this flag so other processes which
    // may be watching it can know.

    StopStreaming();
    DisconnectWS();
}
// for streaming or other data retrievals

void RemoteDataSource::UseSymbols(const std::vector<std::string>& symbols)
{
    symbol_list = symbols;

    // make all the sybolos upper case to be consistent

    rng::for_each(symbol_list, [](auto& symbol) { rng::for_each(symbol, [](char& c) { c = std::toupper(c); }); });
}

std::string RemoteDataSource::RequestData(const std::string& request_string)
{
    // if any problems occur here, we'll just let beast throw an exception.

    // just grab the code from the example program

    net::io_context ioc;
    ssl::context ctx{ssl::context::tlsv12_client};

    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    auto tmp_host = host;
    auto tmp_port = port;
    tcp::resolver resolver{ioc};

    if (!SSL_set_tlsext_host_name(stream.native_handle(), tmp_host.c_str()))
    {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        throw beast::system_error{ec};
    }

    auto const results = resolver.resolve(tmp_host, tmp_port);
    beast::get_lowest_layer(stream).connect(results);
    stream.handshake(ssl::stream_base::client);

    // const std::string request_string =
    //     std::format("https://{}/api/eod/{}.US?from={}&to={}&order={}&period=d&api_token={}&fmt=csv", host_,
    //     symbol,
    //                 start_date, end_date, (sort_asc == UpOrDown::e_Up ? "a" : "d"), api_key_);

    http::request<http::string_body> req{http::verb::get, request_string, version};
    req.set(http::field::host, tmp_host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    http::write(stream, req);

    beast::flat_buffer buffer;

    http::response<http::string_body> res;

    http::read(stream, buffer, res);

    auto result_code = res.result_int();
    BOOST_ASSERT_MSG(result_code == 200,
                     std::format("Failed to retrieve ticker data. Result code: {}\n", result_code).c_str());
    std::string result = res.body();

    // shutdown without causing a 'stream_truncated' error.

    beast::get_lowest_layer(stream).cancel();
    beast::get_lowest_layer(stream).close();

    return result;
}
