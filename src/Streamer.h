// =====================================================================================
//
//       Filename:  Streamer.h
//
//    Description:  Wrapper class for different streaming data sources
//
//        Version:  1.0
//        Created:  03/20/2024 01:46:48 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================

#ifndef _STREAMER_INC_
#define _STREAMER_INC_

#include <cctype>
#include <chrono>
#include <queue>
#include <ranges>
#include <vector>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include <spdlog/spdlog.h>

namespace rng = std::ranges;

// #pragma GCC diagnostic pop

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace net = boost::asio;             // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;        // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;        // from <boost/asio/ip/tcp.hpp>

#include "Uniqueifier.h"
#include "utilities.h"

// =====================================================================================
//        Class:  Streamer
//  Description:  wrapper for streamed data sources
// =====================================================================================
template <class StreamSource>
class Streamer
{
   public:
    struct StreamingEOF
    {
    };

    using Host = UniqType<std::string, struct Host_Tag>;
    using Port = UniqType<std::string, struct Port_Tag>;
    using APIKey = UniqType<std::string, struct API_Key_Tag>;
    using Prefix = UniqType<std::string, struct Prefix_Tag>;

    using TmPt = std::chrono::utc_time<std::chrono::utc_clock::duration>;

    using TopOfBookList = std::vector<TopOfBookOpenAndLastClose>;

    // ====================  LIFECYCLE     =======================================

    ~Streamer()
    {
        // need to disconnect if still connected.

        DisconnectWS();
    }
    Streamer(const Host& host, const Port& port, const APIKey& api_key, const Prefix& prefix)
        : api_key_{api_key.get()},
          host_{host.get()},
          port_{port.get()},
          websocket_prefix_{prefix.get()},
          ctx_{ssl::context::tlsv12_client},
          resolver_{ioc_},
          ws_{ioc_, ctx_}
    {
    }

    Streamer(const Streamer& rhs) = delete;
    Streamer(Streamer&& rhs) = delete;

    // ====================  ACCESSORS     =======================================

    // ====================  MUTATORS      =======================================

    void ConnectWS()
    {
        // the following code is taken from example in Boost documentation

        auto const results = resolver_.resolve(host_, port_);

        auto ep = net::connect(get_lowest_layer(ws_), results);

        if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str()))
        {
            throw beast::system_error(
                beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()),
                "Failed to set SNI Hostname");
        }

        auto host = host_ + ':' + std::to_string(ep.port());

        ws_.set_option(beast::websocket::stream_base::timeout::suggested(beast::role_type::client));

        // Perform the SSL handshake
        ws_.next_layer().handshake(ssl::stream_base::client);

        // Perform the websocket handshake
        ws_.handshake(host, websocket_prefix_);
        BOOST_ASSERT_MSG(ws_.is_open(), "Unable to complete websocket connection.");
    }

    void DisconnectWS()
    {
        if (!ws_.is_open())
        {
            return;
        }
        try
        {
            // beast::close_socket(get_lowest_layer(ws_));
            ws_.close(websocket::close_code::normal);
        }
        catch (std::exception& e)
        {
            spdlog::error("Problem closing socket during disconnect: {}.", e.what());
        }
    }
    void StreamData(bool* had_signal, std::mutex* data_mutex, std::queue<std::string>* streamed_data)
    {
        auto& s_source = static_cast<StreamSource&>(*this);
        s_source.StartStreaming();

        // This buffer will hold the incoming message
        // Each message contains data for a single transaction

        beast::flat_buffer buffer;

        while (ws_.is_open() && !(*had_signal))
        {
            try
            {
                buffer.clear();
                ws_.read(buffer);
                ws_.text(ws_.got_text());
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
                    s_source.StopStreaming();
                    throw StreamingEOF{};
                }
                spdlog::error(std::format("System error. Category: {}. Value: {}. Message: {}", ec.category().name(),
                                          ec.value(), ec.message()));
                *had_signal = true;
                ws_.close(websocket::close_code::going_away);
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
                    s_source.StopStreaming();
                    throw StreamingEOF{};
                }
                *had_signal = true;
                s_source.StopStreaming();
            }
            catch (...)
            {
                spdlog::error("Unknown problem processing steamed data.");
                *had_signal = true;
                s_source.StopStreaming();
            }
        }
        if (*had_signal)
        {
            // do a last check for data

            buffer.clear();
            ws_.read(buffer);
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

        s_source.StopStreaming();
        DisconnectWS();
    }
    // for streaming or other data retrievals

    void UseSymbols(const std::vector<std::string>& symbols)
    {
        symbol_list_ = symbols;

        // make all the sybolos upper case to be consistent

        rng::for_each(symbol_list_, [](auto& symbol) { rng::for_each(symbol, [](char& c) { c = std::toupper(c); }); });
    }

    std::string RequestData(const std::string& request_string)
    {
        // if any problems occur here, we'll just let beast throw an exception.

        // just grab the code from the example program

        net::io_context ioc;
        ssl::context ctx{ssl::context::tlsv12_client};

        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        auto host = host_;
        auto port = port_;
        tcp::resolver resolver{ioc};

        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
        {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        auto const results = resolver.resolve(host, port);
        beast::get_lowest_layer(stream).connect(results);
        stream.handshake(ssl::stream_base::client);

        // const std::string request_string =
        //     std::format("https://{}/api/eod/{}.US?from={}&to={}&order={}&period=d&api_token={}&fmt=csv", host_,
        //     symbol,
        //                 start_date, end_date, (sort_asc == UpOrDown::e_Up ? "a" : "d"), api_key_);

        http::request<http::string_body> req{http::verb::get, request_string, version_};
        req.set(http::field::host, host);
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
    // ====================  OPERATORS     =======================================

    Streamer& operator=(const Streamer& rhs) = delete;
    Streamer& operator=(Streamer&& rhs) = delete;

   protected:
    // ====================  METHODS       =======================================

    // ====================  DATA MEMBERS  =======================================

   private:
    friend StreamSource;

    // ====================  METHODS       =======================================

    Streamer()  // constructor
        : ctx_{ssl::context::tlsv12_client}, resolver_{ioc_}, ws_{ioc_, ctx_}
    {
    }

    // ====================  DATA MEMBERS  =======================================

    net::io_context ioc_;
    ssl::context ctx_;
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<tcp::socket>, false> ws_;
    int version_ = 11;

    std::vector<std::string> symbol_list_;

    std::string host_;
    std::string port_;
    std::string api_key_;
    std::string websocket_prefix_;

};  // ----------  end of template class Streamer  ----------

#endif  // ----- #ifndef _STREAMER_INC_  -----
